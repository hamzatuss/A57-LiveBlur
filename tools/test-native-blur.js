// Termux, supported firmware, root, and an active flagship renderer required.
// Hooks are installed only inside separate app_process test runtimes. Pixel
// tests use a private offscreen layer stack; no visible UI is captured.
const fs=require('fs'),path=require('path'),cp=require('child_process');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'build/native-test');fs.mkdirSync(dir,{recursive:true});
cp.execFileSync(process.env.CXX||'clang++',['-std=c++17','-O2','-fPIC','-shared','-fvisibility=hidden','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-nostdlib++','-Wall','-Wextra','-Werror','-I'+path.join(root,'module/source'),path.join(__dirname,'native-blur-fixture.cpp'),'/system/lib64/liblog.so','-Wl,-z,defs,-z,max-page-size=16384','-o',path.join(dir,'jni_fixture.so')],{stdio:'inherit'});
cp.execFileSync(process.execPath,[path.join(__dirname,'clean_elf.js'),path.join(dir,'jni_fixture.so')],{stdio:'inherit'});
cp.execFileSync(process.execPath,[path.join(__dirname,'make-test-dex.js')],{stdio:'inherit'});
const quote=s=>"'"+s.replaceAll("'","'\\''")+"'";
const cases=[{name:'system:ui'},{name:'com.android.intentresolver'},{name:'com.android.systemui'},{name:'system_server',skip:true,server:true},{name:'system',skip:true},{name:'system:ui:unrelated',skip:true},{name:'com.example.unrelated',skip:true}];
for(const c of cases){
 const env={NATIVEBLUR_TEST_TARGET:c.name,NATIVEBLUR_EXPECT_NO_HOOK:c.skip?'1':'0',NATIVEBLUR_TEST_SERVER:c.server?'1':'0',NATIVEBLUR_MODULE_DIR:path.join(root,'module'),LD_PRELOAD:path.join(dir,'jni_fixture.so'),CLASSPATH:path.join(dir,'fixture.dex')};
 const command=Object.entries(env).map(([k,v])=>k+'='+quote(v)).join(' ')+' /system/bin/app_process /system/bin BlurBridgeTest';
 const r=cp.spawnSync('su',['-c',command],{encoding:'utf8',timeout:30000,maxBuffer:1048576});
 fs.writeFileSync(path.join(dir,c.name.replaceAll(':','_')+'.log'),(r.stdout||'')+(r.stderr||''));
 if(r.status!==0)throw Error(c.name+' failed:\n'+r.stdout+r.stderr);
 if(c.skip&&!r.stdout.includes('Excluded process received no JNI hook.'))throw Error('Process exclusion not verified');
 if(!c.skip&&!r.stdout.includes('Samsung curve=0xffcdcdcd'))throw Error('Samsung curve rendering not verified');
 console.log('PASS '+c.name+': '+(c.skip?'no hook':r.stdout.split('\n').find(s=>s.includes('Offscreen checkerboard'))));
}
