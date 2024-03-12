/*
 * Copyright (c) 2022-2023, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2023, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2024, Tim Ledbetter <timledbetter@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FlyString.h>
#include <AK/URL.h>
#include <LibGfx/Font/UnicodeRange.h>
#include <LibJS/Runtime/ArrayBuffer.h>
#include <LibWeb/Bindings/PlatformObject.h>
#include <LibWeb/Forward.h>
#include <LibWeb/WebIDL/Buffers.h>
#include <LibWeb/WebIDL/ExceptionOr.h>

namespace Web::CSS {

// https://drafts.csswg.org/css-font-loading/#dictdef-fontfacedescriptors
struct FontFaceDescriptors {
    FlyString style { "normal"_string };
    FlyString weight { "normal"_string };
    FlyString stretch { "normal"_string };
    FlyString unicode_range { "U+0-10FFFF"_string };
    FlyString feature_settings { "normal"_string };
    FlyString variation_settings { "normal"_string };
    FlyString display { "auto"_string };
    FlyString ascent_override { "normal"_string };
    FlyString descent_override { "normal"_string };
    FlyString line_gap_override { "normal"_string };
};

class FontFace : public Bindings::PlatformObject {
    WEB_PLATFORM_OBJECT(FontFace, Bindings::PlatformObject);
    JS_DECLARE_ALLOCATOR(FontFace);

public:
    struct Source {
        Variant<String, URL> local_or_url;
        // FIXME: Do we need to keep this around, or is it only needed to discard unwanted formats during parsing?
        Optional<FlyString> format;
    };

    [[nodiscard]] static JS::NonnullGCPtr<FontFace> create(JS::Realm&);
    // Variant<String, JS::Handle<JS::ArrayBuffer>, JS::Handle<WebIDL::ArrayBufferView>>& source
    static WebIDL::ExceptionOr<JS::NonnullGCPtr<FontFace>> construct_impl(JS::Realm& realm, FlyString const& family, FlyString const& source, Optional<FontFaceDescriptors> const& descriptors = {});

    // FontFace(FlyString font_family, Optional<int> weight, Optional<int> slope, Vector<Source> sources, Vector<Gfx::UnicodeRange> unicode_ranges);
    explicit FontFace(JS::Realm&);
    ~FontFace() = default;

    FlyString font_family() const { return m_font_family; }
    Optional<int> weight() const { return m_weight; }
    Optional<int> slope() const { return m_slope; }
    Vector<Source> const& sources() const { return m_sources; }
    Vector<Gfx::UnicodeRange> const& unicode_ranges() const { return m_unicode_ranges; }
    // FIXME: font-stretch, font-feature-settings

private:
    virtual void initialize(JS::Realm&) override;
    virtual void visit_edges(JS::Cell::Visitor&) override;

    FlyString m_font_family;
    Optional<int> m_weight { 0 };
    Optional<int> m_slope { 0 };
    Vector<Source> m_sources;
    Vector<Gfx::UnicodeRange> m_unicode_ranges;
};

}
