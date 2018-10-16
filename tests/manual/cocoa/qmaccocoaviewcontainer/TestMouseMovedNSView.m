/****************************************************************************
 **
 ** Copyright (C) 2016 The Qt Company Ltd.
 ** Contact: https://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:GPL-EXCEPT$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see https://www.qt.io/terms-conditions. For further
 ** information use the contact form at https://www.qt.io/contact-us.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3 as published by the Free Software
 ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
 ** included in the packaging of this file. Please review the following
 ** information to ensure the GNU General Public License requirements will
 ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#import "TestMouseMovedNSView.h"

@implementation TestMouseMovedNSView {
    NSPoint mouseMovedPoint_;
    BOOL wasAcceptingMouseEvents_;
    NSTrackingRectTag trackingRect_;
    NSTrackingArea* trackingArea_;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
        mouseMovedPoint_ = NSMakePoint(50, 50);
    return self;
}

- (void)viewDidMoveToWindow
{
    trackingArea_ = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea_];
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    if (self.window && trackingArea_)
        [self removeTrackingArea:trackingArea_];
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    [self removeTrackingArea: trackingArea_];
    trackingArea_ = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea_];
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)becomeFirstResponder { return YES; }

- (void)mouseEntered:(NSEvent *)theEvent
{
    wasAcceptingMouseEvents_ = [[self window] acceptsMouseMovedEvents];
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window makeFirstResponder:self];
}

- (void)mouseExited:(NSEvent *)theEvent
{
    [self.window setAcceptsMouseMovedEvents:wasAcceptingMouseEvents_];
    [self setNeedsDisplay:YES];
    [self displayIfNeeded];
}

-(void)mouseMoved:(NSEvent *)pTheEvent
{
    mouseMovedPoint_ = [self convertPoint:pTheEvent.locationInWindow fromView:nil];
    [self setNeedsDisplay:YES];
    [self displayIfNeeded];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor whiteColor] set];
    NSRectFill(dirtyRect);

    NSGraphicsContext *nsGraphicsContext = [NSGraphicsContext currentContext];
    CGContextRef cgContextRef = nsGraphicsContext.CGContext;

    CGContextSetRGBStrokeColor(cgContextRef, 0, 0, 0, .5);
    CGContextSetLineWidth(cgContextRef, 1.0);

    CGContextBeginPath(cgContextRef);

    CGContextMoveToPoint(cgContextRef, mouseMovedPoint_.x, 0);
    CGContextAddLineToPoint(cgContextRef, mouseMovedPoint_.x, 1000);

    CGContextMoveToPoint(cgContextRef, 0, mouseMovedPoint_.y);
    CGContextAddLineToPoint(cgContextRef, 1000, mouseMovedPoint_.y);

    CGContextDrawPath(cgContextRef, kCGPathStroke);
}

@end
