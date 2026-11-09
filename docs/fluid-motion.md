# Fluid Motion: notification and Quick Settings inspection

Inspected installed package: `dev.flexy.fluidmotion`, class `ShadeFlyIn`. This was static inspection of its DEX code, with the Xposed module left disabled.

## Hooks

- Before `SecPanelSplitHelper.onPanelExpansionChanged`: captures the split-panel helper and passes the expansion event to `onExpansion`.
- Falls back to `NotificationShadeDepthController.onPanelExpansionChanged` if the split-helper hook finds no methods.
- After `SecPanelWindowBlurBinder` constructors: captures the first View argument as `windowRoot`. This callback does not set blur radius or color curves.
- Before `AndroidComposeView.dispatchDraw`: while QS animation is active on the selected view and the canvas is hardware accelerated, calls `drawPieces` and skips the original draw if that succeeds.

## Notification rows

`apply(Row)` constructs scale/translation matrices and calls `View.setAnimationMatrix`. `stopRows` restores each row with `setAnimationMatrix(null)`. This path could cause moving content and separately calculated blur boundaries to diverge; runtime testing is needed to determine whether Samsung tracks these transforms correctly.

## Quick Settings

`readRects` walks Compose semantics nodes and obtains their bounds in the root. `drawPieces` invokes the original Compose drawing into a recording RenderNode, clips selected rectangles out of the remainder, and replays clipped pieces into individual RenderNodes. Each piece receives its own pivot, scale, translation and alpha. `stopQs` discards the display lists, clears the pieces and invalidates the QS view to restore ordinary drawing.

This replay is the strongest suspected compatibility point: native blur regions may not follow the manually replayed pieces' geometry, clipping and opacity. The inspection did not establish that a conflict occurs, and did not show a direct replacement of the flagship shader or JNI method.

Independent feature settings are `shade_qs_enabled` and `shade_notif_enabled`; both default to true when absent. Animation settings reuse unlock bounce, duration, stagger and fade values, plus shade scale and drop.

No Fluid Motion APK, settings or Xposed enable state was changed during inspection.
