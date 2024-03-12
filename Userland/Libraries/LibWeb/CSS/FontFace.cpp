/*
 * Copyright (c) 2022-2023, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2023, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2024, Tim Ledbetter <timledbetter@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/CSS/FontFace.h>

namespace Web::CSS {

JS_DEFINE_ALLOCATOR(FontFace);

JS::NonnullGCPtr<FontFace> create(JS::Realm& realm)
{
    return realm.heap().allocate<FontFace>(realm, realm);
}

// https://drafts.csswg.org/css-font-loading/#font-face-constructor
WebIDL::ExceptionOr<JS::NonnullGCPtr<FontFace>> FontFace::construct_impl(JS::Realm& realm, FlyString const& family, FlyString const& source, Optional<FontFaceDescriptors> const& descriptors)
{
    // 1. Let font face be a fresh FontFace object. Set font face’s status attribute to "unloaded", Set its internal [[FontStatusPromise]] slot to a fresh pending Promise object.
    // auto font_face = create(realm);
    //(void)font_face;
    (void)realm;
    (void)family;
    (void)source;
    (void)descriptors;
    // 2. If the source argument was a CSSOMString, set font face’s internal [[Urls]] slot to the string.
    //    If the source argument was a BinaryData, set font face’s internal [[Data]] slot to the passed argument.
    // 3. If font face’s [[Data]] slot is not null, queue a task to run the following steps synchronously:
    VERIFY_NOT_REACHED();
}

void FontFace::initialize(JS::Realm& realm)
{
    Base::initialize(realm);
    // set_prototype(&Bindings::ensure_web_prototype<Bindings::FontFacePrototype>(realm, "FontFace"_fly_string));
}

void FontFace::visit_edges(JS::Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    // FIXME
}

// FontFace::FontFace(FlyString font_family, Optional<int> weight, Optional<int> slope, Vector<Source> sources, Vector<Gfx::UnicodeRange> unicode_ranges)
//     : m_font_family(move(font_family))
//     , m_weight(weight)
//     , m_slope(slope)
//     , m_sources(move(sources))
//     , m_unicode_ranges(move(unicode_ranges))
// {
// }

FontFace::FontFace(JS::Realm& realm)
    : PlatformObject(realm)
{
}

}
