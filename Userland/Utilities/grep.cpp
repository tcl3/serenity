/*
 * Copyright (c) 2020, Emanuel Sprung <emanuel.sprung@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Assertions.h>
#include <AK/DeprecatedString.h>
#include <AK/LexicalPath.h>
#include <AK/ScopeGuard.h>
#include <AK/StringBuilder.h>
#include <AK/Vector.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/DirIterator.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibMain/Main.h>
#include <LibRegex/Regex.h>
#include <stdio.h>
#include <unistd.h>

enum class BinaryFileMode {
    Binary,
    Text,
    Skip,
};

struct Options {
    Vector<StringView> files;
    bool recursive { false };
    bool use_ere { false };
    bool fixed_strings { false };
    Vector<DeprecatedString> patterns;
    StringView pattern_file;
    BinaryFileMode binary_mode { BinaryFileMode::Binary };
    bool case_insensitive { false };
    bool line_numbers { false };
    bool invert_match { false };
    bool quiet_mode { false };
    bool suppress_errors { false };
    bool colored_output { false };
    bool count_lines { false };

    bool user_has_specified_files() const { return !files.is_empty(); }
    bool user_specified_multiple_files() const { return files.size() >= 2; }

    static Options default_options(Main::Arguments args)
    {
        auto program_name = AK::LexicalPath::basename(args.strings[0]);
        Options default_options;
        if (program_name == "rgrep"sv)
            default_options.recursive = true;
        else if (program_name == "egrep"sv)
            default_options.use_ere = true;
        else if (program_name == "fgrep"sv)
            default_options.fixed_strings = true;

        default_options.colored_output = isatty(STDOUT_FILENO);

        return default_options;
    }
};

template<typename... Ts>
void fail(StringView format, Ts... args)
{
    warn("\x1b[31m");
    warnln(format, forward<Ts>(args)...);
    warn("\x1b[0m");
    abort();
}

constexpr StringView ere_special_characters = ".^$*+?()[{\\|"sv;
constexpr StringView basic_special_characters = ".^$*[\\"sv;

static DeprecatedString escape_characters(StringView string, StringView characters)
{
    StringBuilder builder;
    for (auto ch : string) {
        if (characters.contains(ch))
            builder.append('\\');

        builder.append(ch);
    }
    return builder.to_deprecated_string();
}

template<typename T>
static int print_matches(Vector<Regex<T>>&& regular_expressions, Options const& options)
{
    size_t matched_line_count = 0;

    for (auto& re : regular_expressions) {
        if (re.parser_result.error != regex::Error::NoError) {
            warnln("regex parse error: {}", regex::get_error_string(re.parser_result.error));
            return 1;
        }
    }

    auto matches = [&](StringView str, StringView filename, size_t line_number, bool print_filename, bool is_binary) {
        size_t last_printed_char_pos { 0 };
        if (is_binary && options.binary_mode == BinaryFileMode::Skip)
            return false;

        for (auto& re : regular_expressions) {
            auto result = re.match(str, PosixFlags::Global);
            if (!(result.success ^ options.invert_match))
                continue;

            if (options.quiet_mode)
                return true;

            if (options.count_lines) {
                matched_line_count++;
                return true;
            }

            if (is_binary && options.binary_mode == BinaryFileMode::Binary) {
                outln(options.colored_output ? "binary file \x1B[34m{}\x1B[0m matches"sv : "binary file {} matches"sv, filename);
            } else {
                if ((result.matches.size() || options.invert_match) && print_filename)
                    out(options.colored_output ? "\x1B[34m{}:\x1B[0m"sv : "{}:"sv, filename);
                if ((result.matches.size() || options.invert_match) && options.line_numbers)
                    out(options.colored_output ? "\x1B[35m{}:\x1B[0m"sv : "{}:"sv, line_number);

                for (auto& match : result.matches) {
                    auto pre_match_length = match.global_offset - last_printed_char_pos;
                    out(options.colored_output ? "{}\x1B[32m{}\x1B[0m"sv : "{}{}"sv,
                        pre_match_length > 0 ? StringView(&str[last_printed_char_pos], pre_match_length) : ""sv,
                        match.view.to_deprecated_string());
                    last_printed_char_pos = match.global_offset + match.view.length();
                }
                auto remaining_length = str.length() - last_printed_char_pos;
                outln("{}", remaining_length > 0 ? StringView(&str[last_printed_char_pos], remaining_length) : ""sv);
            }

            return true;
        }

        return false;
    };

    auto did_match_something = false;

    auto handle_file = [&matches, &options,
                           &matched_line_count, &did_match_something](StringView filename, bool print_filename) -> ErrorOr<void> {
        auto file = TRY(Core::File::open(filename, Core::File::OpenMode::Read));
        auto buffered_file = TRY(Core::InputBufferedFile::create(move(file)));

        for (size_t line_number = 1; TRY(buffered_file->can_read_line()); ++line_number) {
            Array<u8, PAGE_SIZE> buffer;
            auto line = TRY(buffered_file->read_line(buffer));

            auto is_binary = line.contains('\0');

            auto matched = matches(line, filename, line_number, print_filename, is_binary);
            did_match_something = did_match_something || matched;
            if (matched && is_binary && options.binary_mode == BinaryFileMode::Binary)
                break;
        }

        if (options.count_lines && !options.quiet_mode) {
            if (options.user_specified_multiple_files())
                outln("{}:{}", filename, matched_line_count);
            else
                outln("{}", matched_line_count);
            matched_line_count = 0;
        }

        return {};
    };

    auto add_directory = [&handle_file, &options](DeprecatedString base, Optional<DeprecatedString> recursive, auto handle_directory) -> void {
        Core::DirIterator it(recursive.value_or(base), Core::DirIterator::Flags::SkipDots);
        while (it.has_next()) {
            auto path = it.next_full_path();
            if (!FileSystem::is_directory(path)) {
                auto key = options.user_has_specified_files() ? path.view() : path.substring_view(base.length() + 1, path.length() - base.length() - 1);
                if (auto result = handle_file(key, true); result.is_error() && !options.suppress_errors)
                    warnln("Failed with file {}: {}", key, result.release_error());

            } else {
                handle_directory(base, path, handle_directory);
            }
        }
    };

    if (!options.files.size() && !options.recursive) {
        char* line = nullptr;
        size_t line_len = 0;
        ssize_t nread = 0;
        ScopeGuard free_line = [line] { free(line); };
        size_t line_number = 0;
        while ((nread = getline(&line, &line_len, stdin)) != -1) {
            VERIFY(nread > 0);
            if (line[nread - 1] == '\n')
                --nread;
            // Human-readable indexes start at 1, so it's fine to increment already.
            line_number += 1;
            StringView line_view(line, nread);
            bool is_binary = line_view.contains('\0');

            if (is_binary && options.binary_mode == BinaryFileMode::Skip)
                return 1;

            auto matched = matches(line_view, "stdin"sv, line_number, false, is_binary);
            did_match_something = did_match_something || matched;
            if (matched && is_binary && options.binary_mode == BinaryFileMode::Binary)
                break;
        }

        if (options.count_lines && !options.quiet_mode)
            outln("{}", matched_line_count);
    } else {
        if (options.recursive) {
            if (options.user_has_specified_files()) {
                for (auto& filename : options.files) {
                    add_directory(filename, {}, add_directory);
                }
            } else {
                add_directory(".", {}, add_directory);
            }

        } else {
            bool print_filename = options.user_specified_multiple_files();
            for (auto const& filename : options.files) {
                auto result = handle_file(filename, print_filename);
                if (result.is_error()) {
                    if (!options.suppress_errors)
                        warnln("Failed with file {}: {}", filename, result.release_error());
                    return 1;
                }
            }
        }
    }

    return did_match_something ? 0 : 1;
}

ErrorOr<int> serenity_main(Main::Arguments args)
{
    TRY(Core::System::pledge("stdio rpath"));

    Options options = Options::default_options(args);

    Core::ArgsParser args_parser;
    args_parser.add_option(options.recursive, "Recursively scan files", "recursive", 'r');
    args_parser.add_option(options.use_ere, "Extended regular expressions", "extended-regexp", 'E');
    args_parser.add_option(options.fixed_strings, "Treat pattern as a string, not a regexp", "fixed-strings", 'F');
    args_parser.add_option(Core::ArgsParser::Option {
        .argument_mode = Core::ArgsParser::OptionArgumentMode::Required,
        .help_string = "Pattern",
        .long_name = "regexp",
        .short_name = 'e',
        .value_name = "Pattern",
        .accept_value = [&](StringView str) {
            options.patterns.append(str);
            return true;
        },
    });
    args_parser.add_option(Core::ArgsParser::Option {
        .argument_mode = Core::ArgsParser::OptionArgumentMode::Required,
        .help_string = "Read patterns from a file",
        .long_name = "file",
        .short_name = 'f',
        .value_name = "File",
        .accept_value = [&](StringView str) {
            options.pattern_file = str;
            return true;
        },
    });
    args_parser.add_option(options.case_insensitive, "Make matches case-insensitive", nullptr, 'i');
    args_parser.add_option(options.line_numbers, "Output line-numbers", "line-numbers", 'n');
    args_parser.add_option(options.invert_match, "Select non-matching lines", "invert-match", 'v');
    args_parser.add_option(options.quiet_mode, "Do not write anything to standard output", "quiet", 'q');
    args_parser.add_option(options.suppress_errors, "Suppress error messages for nonexistent or unreadable files", "no-messages", 's');
    args_parser.add_option(Core::ArgsParser::Option {
        .argument_mode = Core::ArgsParser::OptionArgumentMode::Required,
        .help_string = "Action to take for binary files ([binary], text, skip)",
        .long_name = "binary-mode",
        .accept_value = [&](StringView str) {
            if ("text"sv == str)
                options.binary_mode = BinaryFileMode::Text;
            else if ("binary"sv == str)
                options.binary_mode = BinaryFileMode::Binary;
            else if ("skip"sv == str)
                options.binary_mode = BinaryFileMode::Skip;
            else
                return false;
            return true;
        },
    });
    args_parser.add_option(Core::ArgsParser::Option {
        .argument_mode = Core::ArgsParser::OptionArgumentMode::None,
        .help_string = "Treat binary files as text (same as --binary-mode text)",
        .long_name = "text",
        .short_name = 'a',
        .accept_value = [&](auto) {
            options.binary_mode = BinaryFileMode::Text;
            return true;
        },
    });
    args_parser.add_option(Core::ArgsParser::Option {
        .argument_mode = Core::ArgsParser::OptionArgumentMode::None,
        .help_string = "Ignore binary files (same as --binary-mode skip)",
        .long_name = nullptr,
        .short_name = 'I',
        .accept_value = [&](auto) {
            options.binary_mode = BinaryFileMode::Skip;
            return true;
        },
    });
    args_parser.add_option(Core::ArgsParser::Option {
        .argument_mode = Core::ArgsParser::OptionArgumentMode::Required,
        .help_string = "When to use colored output for the matching text ([auto], never, always)",
        .long_name = "color",
        .short_name = 0,
        .value_name = "WHEN",
        .accept_value = [&](StringView str) {
            if ("never"sv == str)
                options.colored_output = false;
            else if ("always"sv == str)
                options.colored_output = true;
            else if ("auto"sv != str)
                return false;
            return true;
        },
    });
    args_parser.add_option(options.count_lines, "Output line count instead of line contents", "count", 'c');
    args_parser.add_positional_argument(options.files, "File(s) to process", "file", Core::ArgsParser::Required::No);
    args_parser.parse(args);

    if (!options.pattern_file.is_empty()) {
        auto file = TRY(Core::File::open(options.pattern_file, Core::File::OpenMode::Read));
        auto buffered_file = TRY(Core::InputBufferedFile::create(move(file)));
        Array<u8, PAGE_SIZE> buffer;
        while (!buffered_file->is_eof()) {
            auto next_pattern = TRY(buffered_file->read_line(buffer));
            // Empty lines represent a valid pattern, but the trailing newline
            // should be ignored.
            if (next_pattern.is_empty() && buffered_file->is_eof())
                break;
            options.patterns.append(next_pattern.to_deprecated_string());
        }
    }

    // mock grep behavior: if -e is omitted, use first positional argument as pattern
    if (options.patterns.size() == 0 && options.files.size())
        options.patterns.append(options.files.take_first());

    PosixOptions posix_options {};
    if (options.case_insensitive)
        posix_options |= PosixFlags::Insensitive;

    if (options.use_ere) {
        Vector<Regex<PosixExtended>> regular_expressions;
        for (auto const& pattern : options.patterns) {
            auto escaped_pattern = (options.fixed_strings) ? escape_characters(pattern, ere_special_characters) : pattern;
            regular_expressions.append(Regex<PosixExtended>(escaped_pattern, posix_options));
        }
        return print_matches(move(regular_expressions), options);
    }

    Vector<Regex<PosixBasic>> regular_expressions;
    for (auto const& pattern : options.patterns) {
        auto escaped_pattern = (options.fixed_strings) ? escape_characters(pattern, basic_special_characters) : pattern;
        regular_expressions.append(Regex<PosixBasic>(escaped_pattern, posix_options));
    }
    return print_matches(move(regular_expressions), options);
}
