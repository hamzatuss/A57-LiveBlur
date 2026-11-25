// Builds an optional Interpreter preview from separately supplied donor files.
// Never modifies the production module, installs APKs, or restarts services.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),input=process.argv[2];
if(!input)throw Error('Usage: node tools/build-ai-preview.js /path/to/verified-donor-files');
const hashes={
 'BixbyInterpreter.apk':'5864bf3f448cdc85305f588db9780d8e06d2dd7d19bbfd4de2392d3a423f0a22',
 'privapp-permissions-com.samsung.android.app.interpreter.xml':'d08fe8d6b9134c4c694c0205fc86cb1617aba12f11218d1cc415b537ffa2b742',
 'signature-permissions-com.samsung.android.app.interpreter.xml':'32ef516780a5d071f14c299dc3972964a12cb57491182499b49824bb5e21ed7f'
};
for(const [name,hash]of Object.entries(hashes))if(crypto.createHash('sha256').update(fs.readFileSync(path.join(input,name))).digest('hex')!==hash)throw Error('Donor checksum mismatch: '+name);
fs.mkdirSync(path.join(root,'build'),{recursive:true});const stage=fs.mkdtempSync(path.join(root,'build/ai-preview.')),mod=path.join(stage,'module');
fs.cpSync(path.join(root,'module'),mod,{recursive:true});fs.mkdirSync(path.join(stage,'tools'));fs.copyFileSync(path.join(root,'tools/package.js'),path.join(stage,'tools/package.js'));
function change(file,from,to){const p=path.join(mod,file),s=fs.readFileSync(p,'utf8');if(!s.includes(from))throw Error('Missing preview anchor: '+file);fs.writeFileSync(p,s.replace(from,to));}
for(const file of ['system/etc/floating_feature.xml','system/vendor/etc/floating_feature.xml']){
 change(file,'<SEC_FLOATING_FEATURE_AUDIO_CONFIG_INTERPRETER>0</SEC_FLOATING_FEATURE_AUDIO_CONFIG_INTERPRETER>','<SEC_FLOATING_FEATURE_AUDIO_CONFIG_INTERPRETER>1</SEC_FLOATING_FEATURE_AUDIO_CONFIG_INTERPRETER>');
 const xml=path.join(mod,file);let data=fs.readFileSync(xml,'utf8');
 const key='SEC_FLOATING_FEATURE_COMMON_DISABLE_NATIVE_AI';
 if(data.includes('<'+key+'>'))data=data.replace(new RegExp('<'+key+'>[^<]*</'+key+'>'),'<'+key+'>FALSE</'+key+'>');
 else data=data.replace('</SecFloatingFeatureSet>','    <'+key+'>FALSE</'+key+'>\n</SecFloatingFeatureSet>');
 fs.writeFileSync(xml,data);
 change(file,'</SecFloatingFeatureSet>','    <SEC_FLOATING_FEATURE_COMMON_CONFIG_AI_VERSION>20263</SEC_FLOATING_FEATURE_COMMON_CONFIG_AI_VERSION>\n</SecFloatingFeatureSet>');
}
const app=path.join(mod,'system/priv-app/BixbyInterpreter');fs.mkdirSync(app,{recursive:true});fs.copyFileSync(path.join(input,'BixbyInterpreter.apk'),path.join(app,'BixbyInterpreter.apk'));
const permissions=path.join(mod,'system/etc/permissions');fs.mkdirSync(permissions,{recursive:true});for(const name of Object.keys(hashes).filter(n=>n.endsWith('.xml')))fs.copyFileSync(path.join(input,name),path.join(permissions,name));
change('module.prop','name=Samsung Native Live Blur','name=Samsung Native Live Blur (AI Preview)');
change('module.prop','versionCode=12','versionCode=13');
const prop=path.join(mod,'module.prop');fs.writeFileSync(prop,fs.readFileSync(prop,'utf8').replace(/^description=.*$/m,'description=EXPERIMENTAL Interpreter preview with HighEnd launcher animations and native blur. Enables global AI gates; speech/language downloads and real-app compatibility are unverified. Same module ID; replaces regular build.'));
fs.writeFileSync(path.join(mod,'AI_PREVIEW.json'),JSON.stringify({experimental:true,scope:'Interpreter only, not a full Galaxy AI port',donor:'SM-S942B S942BXXU5BZJA',files:hashes,featureChanges:{SEC_FLOATING_FEATURE_AUDIO_CONFIG_INTERPRETER:'1',SEC_FLOATING_FEATURE_COMMON_DISABLE_NATIVE_AI:'FALSE',SEC_FLOATING_FEATURE_COMMON_CONFIG_AI_VERSION:'20263'},omitted:['SamsungAiCore (slsi_s5e9965 build; target s5e8865 has no advertised NPU feature)','3B language model flags and stub','camera/audio hardware settings','device model spoofing','SELinux policy changes'],validation:'Signed APK and native dependency preflight only; no installation or speech session tested.'},null,2)+'\n');
fs.appendFileSync(path.join(mod,'README.txt'),'\nEXPERIMENTAL AI PREVIEW: includes donor Interpreter and its original permission lists. Global AI gates are enabled; other apps may expose unavailable AI features. Not a complete Galaxy AI port. No speech session has been tested. Reinstall the regular module and reboot to restore regular feature settings.\n');
cp.execFileSync(process.execPath,[path.join(stage,'tools/package.js')],{stdio:'inherit'});
const version=fs.readFileSync(prop,'utf8').match(/^version=(.*)$/m)[1],out=path.join(root,'dist','Samsung-Native-Live-Blur-v'+version+'-AI-Preview.zip');fs.copyFileSync(path.join(stage,'dist','Samsung-Native-Live-Blur-v'+version+'.zip'),out);
fs.writeFileSync(out+'.sha256.txt',crypto.createHash('sha256').update(fs.readFileSync(out)).digest('hex')+'  '+path.basename(out)+'\n');console.log('Optional preview only: '+out);
