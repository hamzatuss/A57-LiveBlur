package com.samsung.android.uniform.widget.clock;
import android.content.Context;
import android.view.View;
/** Selection/state fixture only; not a replacement for Samsung rendering. */
public class AbsClockView extends View {
 private Boolean u=Boolean.TRUE;private float t0;
 public boolean internalAnimating;
 public boolean isClockInternalAnimationRunning(){return internalAnimating;}
 public float amount;public final float editorOpacity=0f;
 public AbsClockView(Context context){super(context);}
 public void setDozeAmount(float value){amount=value;t0=value;}
 public void setBlurEffectEnabled(boolean value){u=value;}
 public boolean blurEnabled(){return u;}
}
