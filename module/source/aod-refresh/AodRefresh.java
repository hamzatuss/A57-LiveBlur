package dev.a57.liveblur;

import android.content.*;
import android.hardware.display.DisplayManager;
import android.os.*;
import android.view.Display;
import java.lang.reflect.Method;

/** Bounded animation-only Samsung refresh votes; no preferences are written. */
public final class AodRefresh {
    static final String TAG="A57AodRefresh";
    final Context context;
    final Handler handler;
    final PowerManager power;
    final PowerManager.WakeLock guard;
    final DisplayManager displays;
    final Object manager;
    final Method acquireMin,acquireMax,release;
    final IBinder owner=new Binder();
    final IBinder settingsOwner=new Binder();
    Object settingsProvider;Method settingsCall;
    Object vote;
    final Runnable end=this::releaseVote;

    AodRefresh(Context c)throws Exception {
        context=c;handler=new Handler(Looper.getMainLooper());
        power=(PowerManager)c.getSystemService(Context.POWER_SERVICE);
        guard=power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,TAG);guard.setReferenceCounted(false);
        displays=(DisplayManager)c.getSystemService(Context.DISPLAY_SERVICE);
        IBinder binder=(IBinder)Class.forName("android.os.ServiceManager").getMethod("getService",String.class).invoke(null,"display");
        Class<?> stub=Class.forName("android.hardware.display.IDisplayManager$Stub");
        manager=stub.getMethod("asInterface",IBinder.class).invoke(null,binder);
        Class<?> api=Class.forName("android.hardware.display.IDisplayManager");
        acquireMin=api.getMethod("acquireRefreshRateMinLimitToken",IBinder.class,int.class,String.class);
        acquireMax=api.getMethod("acquireRefreshRateMaxLimitToken",IBinder.class,int.class,String.class);
        release=Class.forName("com.samsung.android.hardware.display.IRefreshRateToken").getMethod("release");
    }
    int refreshMode(){
        try {
            if(settingsProvider==null){
                Object am=Class.forName("android.app.ActivityManager").getMethod("getService").invoke(null);
                Object holder=Class.forName("android.app.IActivityManager").getMethod("getContentProviderExternal",String.class,int.class,IBinder.class,String.class).invoke(am,"settings",0,settingsOwner,TAG);
                settingsProvider=holder.getClass().getField("provider").get(holder);
                settingsCall=Class.forName("android.content.IContentProvider").getMethod("call",android.content.AttributionSource.class,String.class,String.class,String.class,Bundle.class);
            }
            Bundle extras=new Bundle();extras.putInt("_user",0);
            android.content.AttributionSource attribution=new android.content.AttributionSource.Builder(1000).setPackageName("android").build();
            Bundle result=(Bundle)settingsCall.invoke(settingsProvider,attribution,"settings","GET_secure","refresh_rate_mode",extras);
            String value=result.getString("value");return value==null?1:Integer.parseInt(value);
        }catch(Exception e){android.util.Log.w(TAG,"Refresh preference unavailable; boost skipped",e);return 0;}
    }
    int targetRate(){
        Display d=displays.getDisplay(Display.DEFAULT_DISPLAY);if(d==null)return 0;
        int max=0;for(Display.Mode mode:d.getSupportedModes())if(mode.getPhysicalWidth()==d.getMode().getPhysicalWidth()&&mode.getPhysicalHeight()==d.getMode().getPhysicalHeight())max=Math.max(max,Math.round(mode.getRefreshRate()));
        return refreshMode()==0?Math.min(max,60):max;
    }
    void releaseVote(){
        boolean held=vote!=null||guard.isHeld();
        handler.removeCallbacks(end);
        if(vote!=null){try{release.invoke(vote);}catch(Exception e){android.util.Log.w(TAG,"Vote release failed; shutting down",e);System.exit(1);}finally{vote=null;}}
        if(guard.isHeld())guard.release();
        if(held)android.util.Log.d(TAG,"Animation vote released");
    }
    void boost(){
        releaseVote();if(power.isPowerSaveMode())return;
        int rate=targetRate();if(rate<=60)return;
        try{guard.acquire(1800);vote=acquireMin.invoke(manager,owner,rate,TAG);if(vote!=null){handler.postDelayed(end,1500);android.util.Log.i(TAG,"Animation refresh vote "+rate+"Hz for at most 1500ms");}else releaseVote();}
        catch(Exception e){releaseVote();android.util.Log.w(TAG,"Refresh vote unavailable",e);}
    }
    boolean lastInteractive;
    final Runnable poll=new Runnable(){public void run(){checkInteractive();handler.postDelayed(this,100);}};
    void checkInteractive(){boolean now=power.isInteractive();if(now!=lastInteractive){lastInteractive=now;boost();}if(power.isPowerSaveMode())releaseVote();}
    void run(){
        lastInteractive=power.isInteractive();
        displays.registerDisplayListener(new DisplayManager.DisplayListener(){
            public void onDisplayAdded(int id){} public void onDisplayRemoved(int id){}
            public void onDisplayChanged(int id){if(id==Display.DEFAULT_DISPLAY)checkInteractive();}
        },handler);
        handler.post(poll);
        android.util.Log.i(TAG,"Animation refresh listener ready; idle AOD unchanged");
    }
    void probe()throws Exception {
        Object cap=null;
        try{
            int rate=targetRate();if(rate<=60)throw new IllegalStateException("High refresh mode not selected");
            cap=acquireMax.invoke(manager,new Binder(),60,TAG+"Probe");if(cap==null)throw new IllegalStateException("Max vote unavailable");
            Thread.sleep(450);System.out.println("PROBE capped="+displays.getDisplay(0).getRefreshRate());
            vote=acquireMin.invoke(manager,owner,rate,TAG);if(vote==null)throw new IllegalStateException("Min vote unavailable");
            Thread.sleep(1000);
            java.lang.Process dump=new ProcessBuilder("/system/bin/dumpsys","display").start();
            java.io.BufferedReader reader=new java.io.BufferedReader(new java.io.InputStreamReader(dump.getInputStream()));String line;int count=0;while((line=reader.readLine())!=null){if((line.contains("PRIORITY_REFRESH_RATE_MIN_LIMIT")||line.contains("PRIORITY_REFRESH_RATE_MAX_LIMIT")||line.contains("mDesiredDisplayModeSpecs=")||line.contains("mActiveRenderFrameRate=")||line.contains("mActiveSfDisplayMode=")||line.contains("A57AodRefresh"))&&count++<8)System.out.println("VOTE "+line.trim());}reader.close();dump.waitFor();
            float boosted=displays.getDisplay(0).getRefreshRate();System.out.println("PROBE boosted="+boosted);
            // The headless client can have a 60fps UID override even on a 120Hz panel.
            // Validate the physical policy and active mode separately from client pacing.
        }finally{releaseVote();if(cap!=null)release.invoke(cap);System.out.println("PROBE votes released");}
    }
    public static void main(String[] args){try{runMain(args);}catch(Throwable t){t.printStackTrace(System.out);System.exit(1);}}
    static void runMain(String[] args)throws Exception {
        if(android.os.Process.myUid()==0){android.system.Os.setgid(1000);android.system.Os.setuid(1000);}
        if(android.os.Process.myUid()!=1000)throw new IllegalStateException("Run as system UID");
        Looper.prepareMainLooper();
        Class<?> activityThread=Class.forName("android.app.ActivityThread");
        Object thread=activityThread.getMethod("systemMain").invoke(null);
        Context c=(Context)activityThread.getMethod("getSystemContext").invoke(thread);
        AodRefresh instance=new AodRefresh(c);
        if(args.length==1&&args[0].equals("--probe")){instance.probe();return;}
        if(!android.os.Build.FINGERPRINT.equals("samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys"))return;
        instance.run();
        if(args.length==1&&args[0].equals("--self-test")){instance.boost();if(instance.vote==null)throw new IllegalStateException("Test vote not acquired");instance.handler.postDelayed(()->{if(instance.vote!=null||instance.guard.isHeld()){instance.releaseVote();System.out.println("FAIL: vote exceeded its bound");System.exit(1);}System.out.println("PASS: display listener registered; animation vote and wake guard released on deadline");System.exit(0);},2200);}
        Looper.loop();
    }
}
