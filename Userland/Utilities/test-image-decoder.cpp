/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/Directory.h>
#include <LibCore/MappedFile.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGfx/ImageFormats/JPEGLoader.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments args)
{
    if (args.strings.size() < 2)
        return 1;

    auto success_count = 0;
    auto decode_error_count = 0;
    TRY(Core::Directory::for_each_entry(args.strings[1], Core::DirIterator::SkipParentAndBaseDir, [&](Core::DirectoryEntry const& entry, auto& path) -> ErrorOr<IterationDecision> {
        if (entry.type != Core::DirectoryEntry::Type::File)
            return IterationDecision::Continue;

        auto full_path = TRY(FileSystem::real_path(TRY(String::formatted("{}/{}", path, entry.name))));
        auto file_or_error = Core::MappedFile::map(full_path);
        if (file_or_error.is_error()) {
            warnln("Failed to open file: '{}'.", full_path);
            return IterationDecision::Continue;
        }

        auto file = file_or_error.release_value();
        outln("Trying to decode file: '{}'", full_path);

        auto decoder = Gfx::ImageDecoder::try_create_for_raw_bytes(file->bytes());
        if (!decoder) {
            warnln("Could not find decoder for: '{}'.", full_path);
            return IterationDecision::Continue;
        }

        auto frame_or_error = decoder->frame(0);
        if (frame_or_error.is_error()) {
            warnln("Failed to decode: {}. Error: {}", full_path, frame_or_error.release_error());
            decode_error_count++;
            return IterationDecision::Continue;
        }

        // outln("Successfully decoded: '{}'", full_path);
        success_count++;
        return IterationDecision::Continue;
    }));

    warnln("Successfully decoded: {} files", success_count);
    warnln("{} decoder errors", decode_error_count);

    return 0;
}
