/*
 * Copyright (c) 2018-2022, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2022, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Painting/PaintContext.h>

namespace Web {

static u64 s_next_paint_generation_id = 0;

PaintContext::PaintContext(Painting::RecordingPainter& recording_painter, Palette const& palette, double device_pixels_per_css_pixel)
    : m_recording_painter(recording_painter)
    , m_palette(palette)
    , m_device_pixels_per_css_pixel(device_pixels_per_css_pixel)
    , m_paint_generation_id(s_next_paint_generation_id++)
{
}

CSSPixelRect PaintContext::css_viewport_rect() const
{
    return {
        m_device_viewport_rect.x().value() / m_device_pixels_per_css_pixel,
        m_device_viewport_rect.y().value() / m_device_pixels_per_css_pixel,
        m_device_viewport_rect.width().value() / m_device_pixels_per_css_pixel,
        m_device_viewport_rect.height().value() / m_device_pixels_per_css_pixel
    };
}

DevicePixels PaintContext::rounded_device_pixels(CSSPixels css_pixels) const
{
    return AK::round(css_pixels.to_double() * m_device_pixels_per_css_pixel);
}

DevicePixels PaintContext::enclosing_device_pixels(CSSPixels css_pixels) const
{
    return AK::ceil(css_pixels.to_double() * m_device_pixels_per_css_pixel);
}

DevicePixels PaintContext::floored_device_pixels(CSSPixels css_pixels) const
{
    return AK::floor(css_pixels.to_double() * m_device_pixels_per_css_pixel);
}

DevicePixelPoint PaintContext::rounded_device_point(CSSPixelPoint point) const
{
    return {
        AK::round(point.x().to_double() * m_device_pixels_per_css_pixel),
        AK::round(point.y().to_double() * m_device_pixels_per_css_pixel)
    };
}

DevicePixelPoint PaintContext::floored_device_point(CSSPixelPoint point) const
{
    return {
        AK::floor(point.x().to_double() * m_device_pixels_per_css_pixel),
        AK::floor(point.y().to_double() * m_device_pixels_per_css_pixel)
    };
}

DevicePixelRect PaintContext::enclosing_device_rect(CSSPixelRect rect) const
{
    return {
        AK::floor(rect.x().to_double() * m_device_pixels_per_css_pixel),
        AK::floor(rect.y().to_double() * m_device_pixels_per_css_pixel),
        AK::ceil(rect.width().to_double() * m_device_pixels_per_css_pixel),
        AK::ceil(rect.height().to_double() * m_device_pixels_per_css_pixel)
    };
}

DevicePixelRect PaintContext::rounded_device_rect(CSSPixelRect rect) const
{
    return {
        AK::round(rect.x().to_double() * m_device_pixels_per_css_pixel),
        AK::round(rect.y().to_double() * m_device_pixels_per_css_pixel),
        AK::round(rect.width().to_double() * m_device_pixels_per_css_pixel),
        AK::round(rect.height().to_double() * m_device_pixels_per_css_pixel)
    };
}

DevicePixelSize PaintContext::enclosing_device_size(CSSPixelSize size) const
{
    return {
        AK::ceil(size.width().to_double() * m_device_pixels_per_css_pixel),
        AK::ceil(size.height().to_double() * m_device_pixels_per_css_pixel)
    };
}

DevicePixelSize PaintContext::rounded_device_size(CSSPixelSize size) const
{
    return {
        AK::round(size.width().to_double() * m_device_pixels_per_css_pixel),
        AK::round(size.height().to_double() * m_device_pixels_per_css_pixel)
    };
}

CSSPixels PaintContext::scale_to_css_pixels(DevicePixels device_pixels) const
{
    return CSSPixels::nearest_value_for(device_pixels.value() / m_device_pixels_per_css_pixel);
}

CSSPixelPoint PaintContext::scale_to_css_point(DevicePixelPoint point) const
{
    return {
        scale_to_css_pixels(point.x()),
        scale_to_css_pixels(point.y())
    };
}

CSSPixelSize PaintContext::scale_to_css_size(DevicePixelSize size) const
{
    return {
        scale_to_css_pixels(size.width()),
        scale_to_css_pixels(size.height())
    };
}

CSSPixelRect PaintContext::scale_to_css_rect(DevicePixelRect rect) const
{
    return {
        scale_to_css_point(rect.location()),
        scale_to_css_size(rect.size())
    };
}

}
