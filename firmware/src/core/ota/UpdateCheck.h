#ifndef UPDATE_CHECK_H
#define UPDATE_CHECK_H

// "Is there a newer version?" on the settings and update pages.
//
// The BROWSER asks GitHub, not the orb. The pages carry a marker with the
// running version and commit, and this script asks the GitHub releases API for
// the release marked Latest and writes one line into the marker. Rejected: the
// orb fetching the newest tag itself. That works with scripts off, but it puts
// an HTTPS fetch, its memory and a rate-limited call into every orb, to save
// the reader nothing: the page is being read on a phone or computer that is on
// the internet anyway.
//
// What follows from that choice:
// - No internet on the reading device, a blocked script, or GitHub's limit
//   (60 unsigned calls an hour per public address) all end the same way:
//   "Could not check", or nothing at all. So the update page also carries the
//   two plain links below, which need no script.
// - The script cannot fetch firmware.bin and hand it to /update: the release
//   file is served without a cross-origin header (measured 2026-11-14), so a
//   page on the orb may not read it. The owner downloads it and uploads it.
// - Nothing GitHub sends is written to the page as text: the version shown is
//   rebuilt from the three numbers in the tag.
//
// Including file must have FIRMWARE_VERSION (Utils.h) and BUILD_COMMIT
// (build_id.h). Tested on the host by tools/test_update_check.py, which reads
// the script out of this file: keep it between the one pair of script tags.

#define UPDATE_CHECK_REPO "scottdube/hot-info-orbs"

// Always the release marked Latest: GitHub redirects these, no script needed
#define UPDATE_CHECK_NOTES_URL "https://github.com/" UPDATE_CHECK_REPO "/releases/latest"
#define UPDATE_CHECK_FILE_URL UPDATE_CHECK_NOTES_URL "/download/firmware.bin"

// Where the line appears: empty and of no height until the script fills it.
// data-p is "s" on the settings page, where a newer version links to /update,
// and "u" on the update page itself. Two plain macros and not one that takes
// the page, so each sits among string literals like any other piece of a page.
#define UPDATE_CHECK_MARKER_FOR(page) \
    "<span id='upd' style='display:block' data-p='" page "' data-r='" UPDATE_CHECK_REPO "' data-v='" FIRMWARE_VERSION "' data-c='" BUILD_COMMIT "'></span>"
#define UPDATE_CHECK_MARKER_SETTINGS UPDATE_CHECK_MARKER_FOR("s")
#define UPDATE_CHECK_MARKER_UPDATE UPDATE_CHECK_MARKER_FOR("u")

// A pointer to a literal, not an array: both pages include this, and the
// linker folds two equal literals into one copy but keeps two arrays
static const char *const UPDATE_CHECK_SCRIPT = R"JS(<script>
function orbUpdateNums(s,prefix){var m=new RegExp('^'+prefix+'(\\d+)\\.(\\d+)\\.(\\d+)$').exec(s||'');return m?[+m[1],+m[2],+m[3]]:null}
function orbUpdateLatest(tag){var n=orbUpdateNums(tag,'firmware-');return n?n.join('.'):''}
function orbUpdateState(run,commit,tag,sha){
var a=orbUpdateNums(run,''),b=orbUpdateNums(tag,'firmware-');
if(!a||!b)return'unknown';
for(var i=0;i<3;i++)if(a[i]!==b[i])return a[i]<b[i]?'newer':'ahead';
if(!/^[0-9a-f]{40}$/.test(sha||''))return'current';
return /^[0-9a-f]{7,40}$/.test(commit||'')&&sha.indexOf(commit)===0?'current':'local'}
function orbUpdateText(state,latest){
if(state==='newer')return'⬆ Version '+latest+' is available.';
if(state==='current')return'✔ This is the newest version.';
if(state==='local')return'Local build, not a release file. Newest release: '+latest+'.';
if(state==='ahead')return'This build is newer than the newest release, '+latest+'.';
return'Could not check for a newer version.'}
(function(){
if(typeof document==='undefined')return;
var e=document.getElementById('upd');if(!e)return;
var d=e.dataset,shown=false;
function show(state,latest){
if(shown)return;shown=true;clearTimeout(t);
e.textContent=orbUpdateText(state,latest);
if(state==='newer')e.style.fontWeight='bold';
if(d.p==='s'&&(state==='newer'||state==='local')){
var a=document.createElement('a');a.href='/update';a.textContent='Update page';
e.appendChild(document.createTextNode(' '));e.appendChild(a)}}
var t=setTimeout(function(){show('unknown','')},10000);
e.textContent='Checking for a newer version…';
if(!window.fetch){show('unknown','');return}
fetch('https://api.github.com/repos/'+d.r+'/releases/latest')
.then(function(r){if(!r.ok)throw 0;return r.json()})
.then(function(j){show(orbUpdateState(d.v,d.c,j.tag_name,j.target_commitish),orbUpdateLatest(j.tag_name))})
.catch(function(){show('unknown','')})
})();
</script>)JS";

#endif
