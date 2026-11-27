package dev.a57.liveblur;

import android.content.*;
import android.database.ContentObserver;
import android.hardware.display.DisplayManager;
import android.os.*;
import android.provider.Settings;
import android.view.*;
import java.lang.reflect.*;
import java.util.*;

/** Repairs transient Samsung clock doze state; never edits the user's style. */
public final class ClockAodGuard {
    static final String TAG="A57ClockAod";
    static ClockAodGuard instance;
    final Context context;
    final PowerManager power;
    final Handler main=new Handler(Looper.getMainLooper());
    final Engine engine=new Engine();
    final FrameBurst frames=new FrameBurst();
    final PowerManager.WakeLock frameGuard;
    boolean failed;
    boolean clockCompatible;
    final Runnable apply=this::refresh;
    final Runnable restore=()->{try{if(!dozing())engine.restore();}catch(Throwable e){android.util.Log.w(TAG,"Clock restore unavailable",e);}};
    final Runnable restoreFrames=this::clearFrames;
    void clearFrames(){try{frames.restore();}catch(Throwable e){android.util.Log.w(TAG,"Frame request restore unavailable",e);}finally{if(frameGuard.isHeld())frameGuard.release();}}
    final Runnable pace=this::requestFrames;
    ClockAodGuard(Context c){context=c;power=(PowerManager)c.getSystemService(Context.POWER_SERVICE);frameGuard=power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,TAG+":frames");frameGuard.setReferenceCounted(false);}
    public static synchronized void install(Context c){
        if(instance!=null||!"com.android.systemui".equals(c.getPackageName()))return;
        instance=new ClockAodGuard(c);
        instance.main.post(()->{try{instance.start();}catch(Throwable e){instance.failed=true;android.util.Log.w(TAG,"Clock guard unavailable; native blur unchanged",e);}});
    }
    boolean dozing(){
        try{return !power.isInteractive()&&Settings.System.getInt(context.getContentResolver(),"aod_mode",0)==1;}
        catch(RuntimeException e){return false;}
    }
    boolean compatibleClock(){try{return context.getPackageManager().getPackageInfo("com.samsung.android.app.aodservice",0).getLongVersionCode()==896703000L;}catch(Exception e){return false;}}
    void changed(){
        if(failed)return;
        clockCompatible=compatibleClock();
        main.removeCallbacks(apply);main.removeCallbacks(restore);
        if(dozing()&&clockCompatible){
            // Bounded rechecks cover a clock attached after the sleep callback.
            for(int delay:new int[]{350,750,1400,2200})main.postDelayed(apply,delay);
        }else main.postDelayed(restore,650);
        main.removeCallbacks(pace);main.post(pace);main.postDelayed(pace,150);
    }
    void start(){
        clockCompatible=compatibleClock();
        if(!clockCompatible)android.util.Log.w(TAG,"Clock state correction skipped: unsupported AOD APK version; View pacing still available");
        context.getContentResolver().registerContentObserver(Settings.System.getUriFor("aod_show_state"),false,new ContentObserver(main){public void onChange(boolean self){changed();}});
        IntentFilter f=new IntentFilter();f.addAction(Intent.ACTION_SCREEN_OFF);f.addAction(Intent.ACTION_SCREEN_ON);
        context.registerReceiver(new BroadcastReceiver(){public void onReceive(Context c,Intent i){changed();}},f,Context.RECEIVER_NOT_EXPORTED);
        changed();android.util.Log.i(TAG,"Samsung clock doze-state guard ready");
    }
    @SuppressWarnings("unchecked")
    ArrayList<View> roots()throws Exception {
        Class<?> w=Class.forName("android.view.WindowManagerGlobal");
        Object wm=w.getMethod("getInstance").invoke(null);
        Field lock=w.getDeclaredField("mLock"),views=w.getDeclaredField("mViews");lock.setAccessible(true);views.setAccessible(true);
        synchronized(lock.get(wm)){return new ArrayList<>((ArrayList<View>)views.get(wm));}
    }
    void refresh(){
        if(failed||!clockCompatible||!dozing())return;
        if(!compatibleClock()){clockCompatible=false;return;}
        try{int count=engine.apply(roots());if(count>0)android.util.Log.d(TAG,"Applied AOD clock endpoint to "+count+" clock(s)");}
        catch(Throwable e){failed=true;android.util.Log.w(TAG,"Clock guard stopped; no settings changed",e);}
    }
    void requestFrames(){
        if(failed)return;
        try{
            if(power.isPowerSaveMode()||Settings.Secure.getInt(context.getContentResolver(),"refresh_rate_mode",1)==0)return;
            DisplayManager manager=(DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);Display d=manager.getDisplay(Display.DEFAULT_DISPLAY);if(d==null)return;
            float rate=0;for(Display.Mode mode:d.getSupportedModes())if(mode.getPhysicalWidth()==d.getMode().getPhysicalWidth()&&mode.getPhysicalHeight()==d.getMode().getPhysicalHeight())rate=Math.max(rate,mode.getRefreshRate());
            if(rate<=60)return;
            frameGuard.acquire(1800);
            int count=frames.request(roots(),rate);
            main.removeCallbacks(restoreFrames);if(count>0)main.postDelayed(restoreFrames,1500);else clearFrames();
            if(count>0)android.util.Log.i(TAG,"View pacing requested "+rate+"fps for "+count+" root(s), bounded to 1500ms");
        }catch(Throwable e){main.removeCallbacks(restoreFrames);restoreFrames.run();android.util.Log.w(TAG,"View pacing unavailable; clock guard retained",e);}
    }
    static final class FrameBurst {
        final WeakHashMap<View,Float> previous=new WeakHashMap<>();
        float requested;
        int request(Iterable<View> roots,float rate){int count=0;requested=rate;for(View v:roots){if(!previous.containsKey(v))previous.put(v,v.getRequestedFrameRate());v.setRequestedFrameRate(rate);v.invalidate();if(Math.abs(v.getRequestedFrameRate()-rate)<0.1f)count++;}return count;}
        void restore(){for(Map.Entry<View,Float> e:new ArrayList<>(previous.entrySet()))if(Math.abs(e.getKey().getRequestedFrameRate()-requested)<0.1f){e.getKey().setRequestedFrameRate(e.getValue());e.getKey().invalidate();}previous.clear();}
    }
    static final class Engine {
        static final class Access {
            final Method doze,blur,running;final Field enabled,amount;
            Access(Class<?> cls,Class<?> base)throws ReflectiveOperationException {
                doze=cls.getMethod("setDozeAmount",float.class);blur=cls.getMethod("setBlurEffectEnabled",boolean.class);
                enabled=base.getDeclaredField("u");amount=base.getDeclaredField("t0");
                if(enabled.getType()!=Boolean.class||amount.getType()!=float.class)throw new NoSuchFieldException("Unexpected clock schema");
                enabled.setAccessible(true);amount.setAccessible(true);
                Method m;try{m=cls.getMethod("isClockInternalAnimationRunning");}catch(NoSuchMethodException e){m=null;}running=m;
            }
        }
        final Map<Class<?>,Access> methods=new HashMap<>();
        final WeakHashMap<View,Boolean> touched=new WeakHashMap<>();
        int remaining;
        Access method(Class<?> cls){
            if(methods.containsKey(cls))return methods.get(cls);
            Access access=null;
            for(Class<?> c=cls;c!=null;c=c.getSuperclass())if(c.getName().equals("com.samsung.android.uniform.widget.clock.AbsClockView")){
                try{access=new Access(cls,c);}catch(ReflectiveOperationException ignored){}break;
            }
            methods.put(cls,access);return access;
        }
        boolean scan(View view){
            if(--remaining<0||view.getVisibility()!=View.VISIBLE)return false;
            Access access=method(view.getClass());
            if(access!=null){try{
                if(access.running!=null&&Boolean.TRUE.equals(access.running.invoke(view)))return false;
                Object enabled=access.enabled.get(view);if(!(enabled instanceof Boolean))return false;
                if(!((Boolean)enabled)&&access.amount.getFloat(view)>=0.999f)return false;
                if(!touched.containsKey(view))touched.put(view,(Boolean)enabled);
                access.blur.invoke(view,false);access.doze.invoke(view,1f);return true;
            }catch(ReflectiveOperationException e){android.util.Log.w(TAG,"Clock endpoint rejected",e);return false;}}
            if(view instanceof ViewGroup){ViewGroup group=(ViewGroup)view;for(int i=0;i<group.getChildCount()&&remaining>0;i++)if(scan(group.getChildAt(i)))return true;}
            return false;
        }
        int apply(Iterable<View> roots){int count=0;remaining=2048;for(View root:roots){if(remaining<=0)break;if(scan(root))count++;}return count;}
        void restore(){
            for(View view:new ArrayList<>(touched.keySet())){Access access=method(view.getClass());if(access!=null)try{
                // Leave a state already changed by Samsung's own wake controller alone.
                if(Boolean.FALSE.equals(access.enabled.get(view))&&access.amount.getFloat(view)>=0.999f){access.doze.invoke(view,0f);access.blur.invoke(view,touched.get(view));}
            }catch(ReflectiveOperationException e){android.util.Log.w(TAG,"Clock restore rejected",e);}}
            touched.clear();
        }
    }
}
