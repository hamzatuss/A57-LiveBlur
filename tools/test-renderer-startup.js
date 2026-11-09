// Root-only integration test. All mounts target a temporary file in a private
// mount namespace. Namespace entry, process monitoring and service restarts
// are replaced in fixture copies; installed modules are never modified.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),moduleDir=path.join(root,'module');
if(process.getuid()!==0)throw Error('Run this test as root on the supported phone');
if(!process.argv.includes('--private')){
 const r=cp.spawnSync('/system/bin/unshare',['-m',process.execPath,__filename,'--private'],{stdio:'inherit'});process.exit(r.status??1);
}
cp.execFileSync('/system/bin/mount',['-o','rprivate','none','/']);
if(fs.readlinkSync('/proc/self/ns/mnt')===fs.readlinkSync('/proc/1/ns/mnt'))throw Error('Private mount namespace required');
fs.mkdirSync(path.join(root,'build'),{recursive:true});
const dir=fs.mkdtempSync(path.join(root,'build/renderer-tests.')),bin=path.join(dir,'bin'),target=path.join(dir,'target'),bb=path.join(bin,'bb');
fs.mkdirSync(bin);fs.mkdirSync(path.join(dir,'renderer'));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const conf=fs.readFileSync(path.join(moduleDir,'renderer.conf'),'utf8'),stock=conf.match(/^STOCK_SHA256=(.*)$/m)[1],donor=conf.match(/^DONOR_SHA256=(.*)$/m)[1];
const stockFile=path.join(moduleDir,'backup/surfaceflinger.stock');
fs.copyFileSync(stockFile,target);fs.copyFileSync(path.join(moduleDir,'renderer/surfaceflinger'),path.join(dir,'renderer/surfaceflinger'));fs.writeFileSync(path.join(dir,'renderer.conf'),conf);
function writeExe(p,s){fs.writeFileSync(p,s,{mode:0o755})}
writeExe(bb,`#!/system/bin/sh\nif [ "$1" = nsenter ]; then\n shift\n [ "$1" != -t ] || shift 2\n [ "$1" != -m ] || shift\n exec "$@"\nfi\nexec /data/adb/ksu/bin/busybox "$@"\n`);
for(const name of ['post-fs-data.sh','action.sh','renderer-watch.sh']){let s=fs.readFileSync(path.join(moduleDir,name),'utf8');s=s.replace('BB=/data/adb/ksu/bin/busybox','BB='+bb).replaceAll('/system/bin/surfaceflinger',target);writeExe(path.join(dir,name==='renderer-watch.sh'?'watch-real.sh':name),s)}
writeExe(path.join(dir,'renderer-watch.sh'),'#!/system/bin/sh\nsleep 0.2\n');
const flag=n=>path.join(dir,n),remove=n=>fs.rmSync(flag(n),{force:true}),exists=n=>fs.existsSync(flag(n));
const run=n=>cp.execFileSync('/system/bin/sh',[flag(n)],{encoding:'utf8',env:{...process.env,PATH:'/system/bin'}});
const assert=(v,msg)=>{if(!v)throw Error(msg)};let mounted=false;
function clear(){for(const n of ['renderer-disabled','trial-pending','renderer-watch.pid'])remove(n)}
function unmount(){cp.execFileSync('/system/bin/umount',[target]);mounted=false;assert(hash(target)===stock,'Underlying stock file was not restored')}
try{
 run('post-fs-data.sh');mounted=true;
 assert(hash(target)===donor,'Stock-to-donor startup failed');assert(!exists('renderer-disabled'),'Startup disabled renderer');
 assert(fs.readFileSync(flag('trial-pending'),'utf8').trim()===fs.readFileSync('/proc/sys/kernel/random/boot_id','utf8').trim(),'Trial boot ID missing');
 run('post-fs-data.sh');assert(hash(target)===donor,'Repeated startup lost donor');unmount();
 console.log('PASS: stock startup and same-boot repeat create only one bind');clear();
 cp.execFileSync('/system/bin/mount',['-o','bind',flag('renderer/surfaceflinger'),target]);mounted=true;
 run('post-fs-data.sh');assert(hash(target)===donor,'Existing donor rejected');unmount();
 console.log('PASS: existing flagship renderer adopted without stacking a bind');clear();
 fs.writeFileSync(flag('renderer-disabled'),'');run('post-fs-data.sh');assert(hash(target)===stock,'Disabled renderer was mounted');
 console.log('PASS: explicit disabled marker respected');clear();
 fs.writeFileSync(flag('trial-pending'),'different-boot\n');run('post-fs-data.sh');assert(exists('renderer-disabled')&&hash(target)===stock,'Failed prior boot not guarded');
 console.log('PASS: unfinished previous-boot trial retains stock');clear();
 fs.writeFileSync(target,'unknown renderer');const unknown=hash(target);run('post-fs-data.sh');assert(hash(target)===unknown&&!exists('trial-pending'),'Unknown renderer was replaced');fs.copyFileSync(stockFile,target);
 console.log('PASS: unknown renderer refused');clear();
 run('action.sh');assert(!exists('renderer-disabled')&&!exists('trial-pending')&&hash(target)===stock,'Status Action changed renderer state');
 console.log('PASS: Action is read-only');
 // Exercise the real watchdog, but intercept sleep, PID discovery and the
 // property-service command. Its unmount still uses the real kernel.
 writeExe(path.join(bin,'sleep'),'#!/system/bin/sh\nexit 0\n');writeExe(path.join(bin,'pidof'),'#!/system/bin/sh\nexit 1\n');
 writeExe(path.join(bin,'setprop'),'#!/system/bin/sh\nprintf "%s\\n" "$*" >> "'+flag('restart-request')+'"\n');
 cp.execFileSync('/system/bin/mount',['-o','bind',flag('renderer/surfaceflinger'),target]);mounted=true;
 cp.execFileSync('/system/bin/sh',[flag('watch-real.sh'),dir],{encoding:'utf8',env:{...process.env,PATH:bin+':/system/bin'}});mounted=false;
 assert(exists('renderer-disabled')&&hash(target)===stock,'Watchdog failed to restore stock');assert(fs.readFileSync(flag('restart-request'),'utf8').trim()==='ctl.restart surfaceflinger','Restart request not captured');
 console.log('PASS: watchdog restores stock; live restart command intercepted');clear();
 // Only marker-reset/migration behavior is under test here: package integrity
 // and permissions are validated separately, so these installer helpers are
 // stubbed. Every old-module path points into this fixture directory.
 const stage=flag('stage'),old=flag('old-renderer'),oldUpdate=flag('old-update');fs.mkdirSync(stage);fs.mkdirSync(old);fs.mkdirSync(oldUpdate);
 for(const name of ['renderer-disabled','trial-pending','renderer-watch.pid'])fs.writeFileSync(path.join(stage,name),'stale');
 let installer=fs.readFileSync(path.join(moduleDir,'customize.sh'),'utf8').replaceAll('/system/bin/surfaceflinger',target).replaceAll('/data/adb/modules/native_blur_renderer',old).replaceAll('/data/adb/modules_update/native_blur_renderer',oldUpdate);
 writeExe(flag('installer.sh'),installer);
 const harness='MODPATH='+stage+'\nabort(){ echo "$*" >&2; exit 1; }\nset_perm_recursive(){ :; }\nset_perm(){ :; }\nui_print(){ :; }\nsha256sum(){ if [ "$1" = -c ]; then return 0; fi; /system/bin/sha256sum "$@"; }\n. '+flag('installer.sh')+'\n';
 writeExe(flag('installer-test.sh'),harness);run('installer-test.sh');
 for(const name of ['renderer-disabled','trial-pending','renderer-watch.pid'])assert(!fs.existsSync(path.join(stage,name)),'Installer retained '+name);
 assert(fs.existsSync(path.join(old,'disable'))&&fs.existsSync(path.join(oldUpdate,'disable')),'Old renderer migration markers missing');
 console.log('PASS: staged installer clears old markers and retires standalone renderer');
 console.log('All isolated integration checks passed. Installed modules and live display were not changed.');
}finally{if(mounted)cp.spawnSync('/system/bin/umount',[target]);}
