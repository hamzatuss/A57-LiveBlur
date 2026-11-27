package dev.a57.liveblur;
import android.content.Context;
import android.os.Looper;
import android.view.View;
import android.widget.FrameLayout;
import com.samsung.android.uniform.widget.clock.AbsClockView;
import java.util.Arrays;
public final class ClockGuardFixture {
 public static final class ForeignView extends View {boolean called;public ForeignView(Context c){super(c);}public void setDozeAmount(float n){called=true;}}
 static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
 public static void main(String[] args){try{
  Class<?> shared=Class.forName("com.android.internal.os.ApplicationSharedMemory");shared.getMethod("setInstance",shared).invoke(null,shared.getMethod("create").invoke(null));
  Looper.prepareMainLooper();Class<?> at=Class.forName("android.app.ActivityThread");Object thread=at.getMethod("systemMain").invoke(null);Context c=(Context)at.getMethod("getSystemContext").invoke(thread);
  View frameProbe=new View(c);float oldRate=frameProbe.getRequestedFrameRate();frameProbe.setRequestedFrameRate(120f);System.out.println("Native View pacing API: before="+oldRate+" requested="+frameProbe.getRequestedFrameRate());frameProbe.setRequestedFrameRate(oldRate);
  ClockAodGuard.Engine engine=new ClockAodGuard.Engine();
  FrameLayout homeEntry=new FrameLayout(c);ForeignView foreign=new ForeignView(c);AbsClockView clock=new AbsClockView(c);homeEntry.addView(foreign);homeEntry.addView(clock);
  clock.internalAnimating=true;check(engine.apply(Arrays.asList(homeEntry))==0&&clock.amount==0f,"Active clock animation interrupted");clock.internalAnimating=false;
  check(engine.apply(Arrays.asList(homeEntry))==1,"Clock not found");check(clock.amount==1f&&!clock.blurEnabled()&&clock.editorOpacity==0f,"AOD state or editor opacity changed incorrectly");check(!foreign.called,"Unrelated view modified");
  check(engine.apply(Arrays.asList(homeEntry))==0&&clock.amount==1f,"Corrected clock was reapplied unnecessarily");engine.restore();check(clock.amount==0f&&clock.blurEnabled()&&clock.editorOpacity==0f,"Wake did not restore normal state");
  FrameLayout hidden=new FrameLayout(c);AbsClockView hiddenClock=new AbsClockView(c);hidden.addView(hiddenClock);hidden.setVisibility(View.GONE);check(engine.apply(Arrays.asList(hidden))==0&&hiddenClock.amount==0f,"Hidden clock modified");
  AbsClockView lockEntry=new AbsClockView(c);lockEntry.setDozeAmount(1f);lockEntry.setBlurEffectEnabled(false);check(engine.apply(Arrays.asList(lockEntry))==0&&lockEntry.amount==1f,"Correct native AOD state regressed");engine.restore();check(lockEntry.amount==1f&&!lockEntry.blurEnabled(),"Native-owned AOD state was altered");
  AbsClockView nativeOwner=new AbsClockView(c);engine.apply(Arrays.asList(nativeOwner));nativeOwner.setDozeAmount(0.4f);engine.restore();check(nativeOwner.amount==0.4f&&!nativeOwner.blurEnabled(),"Native owner clock update overwritten");
  FrameLayout large=new FrameLayout(c);for(int i=0;i<2100;i++)large.addView(new View(c));check(engine.apply(Arrays.asList(large))==0&&engine.remaining<=0,"Traversal budget not enforced");
  ClockAodGuard.FrameBurst frames=new ClockAodGuard.FrameBurst();View pacingRoot=new FrameLayout(c);float original=pacingRoot.getRequestedFrameRate();check(frames.request(Arrays.asList(pacingRoot),120f)==1,"Native root pacing request rejected");frames.restore();check(Float.compare(original,pacingRoot.getRequestedFrameRate())==0,"Original pacing value not restored");frames.request(Arrays.asList(pacingRoot),120f);pacingRoot.setRequestedFrameRate(90f);frames.restore();check(pacingRoot.getRequestedFrameRate()==90f,"Intervening owner request overwritten");
  System.out.println("PASS: native View pacing and exact restore/owner-change preservation; active clock animation respected;");
  System.out.println("PASS: home-entry endpoint, repeated attachment, wake restore, zero editor opacity preserved, native AOD endpoint, unrelated/hidden exclusion, bounded traversal");
 }catch(Throwable t){t.printStackTrace(System.out);System.exit(1);}}
}
