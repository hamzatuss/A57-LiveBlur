import dalvik.system.PathClassLoader;
public final class ClockColorProbe {
 public static void main(String[] args){try{
  PathClassLoader cl=new PathClassLoader(args[0],ClassLoader.getSystemClassLoader());
  java.lang.reflect.Method m=cl.loadClass("aod.ly0").getDeclaredMethod("i",float.class,boolean.class,int.class,int.class);m.setAccessible(true);
  int lock=(Integer)m.invoke(null,0f,true,0x00ffffff,0xffffffff);
  int aod=(Integer)m.invoke(null,1f,true,0x00ffffff,0xffffffff);
  System.out.printf("Samsung color interpolation: lock=%08x AOD=%08x%n",lock,aod);
  Class<?> base=cl.loadClass("com.samsung.android.uniform.widget.clock.AbsClockView"), concrete=cl.loadClass("com.samsung.android.uniform.widget.clock.AnalogClockView");
  Class<?> unsafeClass=Class.forName("sun.misc.Unsafe");java.lang.reflect.Field singleton=unsafeClass.getDeclaredField("theUnsafe");singleton.setAccessible(true);Object unsafe=singleton.get(null);
  Object clock=unsafeClass.getMethod("allocateInstance",Class.class).invoke(unsafe,concrete);
  java.lang.reflect.Field enabled=base.getDeclaredField("u"),active=base.getDeclaredField("y"),opacity=base.getDeclaredField("x");enabled.setAccessible(true);active.setAccessible(true);opacity.setAccessible(true);
  enabled.set(clock,Boolean.TRUE);active.setBoolean(clock,true);opacity.setFloat(clock,0f);
  java.lang.reflect.Method color=base.getDeclaredMethod("I",float.class,int.class,int.class);color.setAccessible(true);
  int broken=(Integer)color.invoke(clock,1f,0x00ffffff,0xffffffff);
  enabled.set(clock,Boolean.FALSE);active.setBoolean(clock,false);
  int fixed=(Integer)color.invoke(clock,1f,0x00ffffff,0xffffffff);
  System.out.printf("Actual Samsung clock color with opacity 0: stale glass=%08x doze glass disabled=%08x%n",broken,fixed);
  if((broken>>>24)!=0||(fixed>>>24)!=255)throw new AssertionError("Native clock alpha model did not match");
  if((aod>>>24)!=255)throw new AssertionError("AOD endpoint not opaque");
 }catch(Throwable t){t.printStackTrace(System.out);System.exit(1);}}
}
