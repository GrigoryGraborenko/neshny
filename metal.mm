#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

void* MacOSGetWindowLayer(void* nsWindowPtr) {

  NSWindow* nsWindow = (NSWindow*)nsWindowPtr;
  NSView* view = [nsWindow contentView];
  [view setWantsLayer:YES];
  [view setLayer:[CAMetalLayer layer]];

  [[view layer] setContentsScale:[nsWindow backingScaleFactor]];
  return [view layer];
}