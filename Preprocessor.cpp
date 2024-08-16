////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Neshny {

/*
supported:
    #define A B
    #define A(x) (x * x)
    #define A (without value)
    #undef
    #ifdef
    #else
    #elifdef - else defined
    #endif
    #include
will support:
    #ifndef
    #elifndef - else not defined
will not support:
    #if
*/

////////////////////////////////////////////////////////////////////////////////
bool IsWhiteSpace(char c) {
    return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t');
}

////////////////////////////////////////////////////////////////////////////////
QByteArray Preprocess(QByteArray input, const std::function<QByteArray(std::string_view, std::string&)>& loader, std::string& err_msg) {

    QByteArray output;
    output.reserve(input.size());

    struct ReplaceWords {

        bool operator<(const ReplaceWords& other) const { return p_Word.size() > other.p_Word.size(); }

        QByteArray  p_Word;
        QByteArray  p_Replace;
        int         p_NumArgs;
        std::vector<std::variant<QByteArray, int>> p_FunctionPieces;
    };

    enum class IfDefMode {
        PRE_KEEP,
        KEEP,
        POST_KEEP
    };

    std::list<ReplaceWords> replacements = {}; // needs to always be sorted from longest to shortest
    std::set<QString> includes = {}; // can only include a file once

    bool multi_line_comment = false;
    bool line_comment = false;
    bool ignore_until_newline = false;
    char last_val = 0;
    std::stack<IfDefMode> ifdefs;
    for (int c = 0; c < input.size(); c++) {
        char val = input[c];
        if (ignore_until_newline) {
            if (val == '\n') {
                ignore_until_newline = false;
            }
            continue;
        }
        bool remove_code = (!ifdefs.empty()) && (ifdefs.top() != IfDefMode::KEEP);

        if ((!remove_code) && (!multi_line_comment) && (!line_comment) && (val == '*') && (last_val == '/')) {
            multi_line_comment = true;
        } else if ((!remove_code) && multi_line_comment && (val == '/') && (last_val == '*')) {
            multi_line_comment = false;
        } else if ((!remove_code) && (!multi_line_comment) && (val == '/') && (last_val == '/')) {
            line_comment = true;
        } else if ((!remove_code) && (val == '\n')) {
            line_comment = false;
        } else if ((!multi_line_comment) && (!line_comment) && (val == '#')) {

            int remaining = input.size() - c;
            // #define
            if ((!remove_code) && (remaining >= 7) && (input[c + 1] == 'd') && (input[c + 2] == 'e') && (input[c + 3] == 'f') && (input[c + 4] == 'i') && (input[c + 5] == 'n') && (input[c + 6] == 'e')) {
                int name_start = -1;
                int name_end = -1;
                int def_start = -1;
                int def_end = -1;
                QByteArrayList arg_names;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool newline = curr == '\n';
                    bool whitespace = IsWhiteSpace(curr);
                    if (newline) {
                        if ((name_end < 0) && (name_start >= 0)) {
                            name_end = cc;
                        } else if ((def_end < 0) && (def_start >= 0)) {
                            def_end = cc;
                        }
                        break;
                    } else if ((def_start >= 0) && (!whitespace)) {
                        def_end = cc + 1;
                    } else if ((name_end >= 0) && (def_start < 0) && !whitespace) {
                        def_start = cc;
                    } else if ((name_start >= 0) && (name_end < 0) && (curr == '(')) {
                        QByteArray arg_name = "";
                        name_end = cc;
                        for (cc = cc + 1; cc < input.size(); cc++) {
                            curr = input[cc];
                            if (curr == ')') {
                                arg_names += arg_name;
                                arg_name = "";
                                break;
                            } else if(curr == ',') {
                                arg_names += arg_name;
                                arg_name = "";
                            } else if(curr != ' ') {
                                arg_name += curr;
                            }
                        }
                    } else if ((name_start >= 0) && (name_end < 0) && whitespace) {
                        name_end = cc;
                    } else if((name_start < 0) && !whitespace) {
                        name_start = cc;
                    }
                }
                if (def_end >= 0) {
                    auto new_name = input.mid(name_start, name_end - name_start);
                    for (auto iter = replacements.begin(); iter != replacements.end(); ) {
                        if (iter->p_Word == new_name) {
                            iter = replacements.erase(iter);
                        } else {
                            iter++;
                        }
                    }
                    replacements.push_back({
                        new_name,
                        input.mid(def_start, def_end - def_start),
                        (int)arg_names.size()
                    });
                    if (!arg_names.empty()) {
                        // this formats the function body into something usable
                        auto& pieces = replacements.back().p_FunctionPieces;
                        int first_replace_ind = -1;
                        for (int cc = def_start; cc < def_end; ) {
                            bool found_any = false;
                            for (int a = 0; a < arg_names.size(); a++) {
                                auto& arg = arg_names[a];
                                bool matches = true;
                                for (int ac = 0; ac < arg.size(); ac++) {
                                    int in_ind = cc + ac;
                                    if ((input.size() <= in_ind) || (arg[ac] != input[in_ind])) {
                                        matches = false;
                                        break;
                                    }
                                }
                                if (matches) {
                                    found_any = true;
                                    if (first_replace_ind >= 0) {
                                        pieces.push_back(input.mid(first_replace_ind, cc - first_replace_ind));
                                    }
                                    pieces.push_back(a);
                                    cc += arg.size();
                                    first_replace_ind = -1;
                                    break;
                                }
                            }
                            if (!found_any) {
                                if (first_replace_ind < 0) {
                                    first_replace_ind = cc;
                                }
                                cc++;
                            }
                        }
                        if (first_replace_ind >= 0) {
                            pieces.push_back(input.mid(first_replace_ind, def_end - first_replace_ind));
                        }
                    }
                    ignore_until_newline = true;
                    c = def_end - 1;
                    replacements.sort();
                    continue;
                } else if (name_end >= 0) {
                    replacements.push_back({
                        input.mid(name_start, name_end - name_start),
                        QByteArray(),
                        0
                    });
                    ignore_until_newline = true;
                    c = name_end - 1;
                    replacements.sort();
                    continue;
                }
                replacements.sort();
            // #undef
            } else if ((!remove_code) && (remaining >= 6) && (input[c + 1] == 'u') && (input[c + 2] == 'n') && (input[c + 3] == 'd') && (input[c + 4] == 'e') && (input[c + 5] == 'f')) {

                int name_start = -1;
                int name_end = -1;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = IsWhiteSpace(curr);
                    if ((name_start >= 0) && (name_end < 0) && whitespace) {
                        name_end = cc;
                        break;
                    } else if((name_start < 0) && !whitespace) {
                        name_start = cc;
                    }
                }
                if (name_end >= 0) {
                    auto name = input.mid(name_start, name_end - name_start);
                    bool removed = false;
                    for (auto iter = replacements.begin(); iter != replacements.end(); iter++) {
                        if (iter->p_Word == name) {
                            replacements.erase(iter);
                            c = name_end - 1;
                            removed = true;
                            ignore_until_newline = true;
                            break;
                        }
                    }
                    if (removed) {
                        continue;
                    }
                }
            // #ifdef
            } else if ((!remove_code) && (remaining >= 6) && (input[c + 1] == 'i') && (input[c + 2] == 'f') && (input[c + 3] == 'd') && (input[c + 4] == 'e') && (input[c + 5] == 'f')) {

                ignore_until_newline = true;
                int name_start = -1;
                int name_end = -1;
                for (int cc = c + 6; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = IsWhiteSpace(curr);
                    if ((name_start >= 0) && (name_end < 0) && whitespace) {
                        name_end = cc;
                        break;
                    }
                    else if ((name_start < 0) && !whitespace) {
                        name_start = cc;
                    }
                }
                if (name_end >= 0) {
                    auto name = input.mid(name_start, name_end - name_start);
                    bool found = false;
                    for (auto& replace : replacements) {
                        if (replace.p_Word == name) {
                            found = true;
                            break;
                        }
                    }
                    ifdefs.push(found ? IfDefMode::KEEP : IfDefMode::PRE_KEEP);
                }
                continue;
            // #elifdef
            } else if ((!ifdefs.empty()) && (remaining >= 8) && (input[c + 1] == 'e') && (input[c + 2] == 'l') && (input[c + 3] == 'i') && (input[c + 4] == 'f') && (input[c + 5] == 'd') && (input[c + 6] == 'e') && (input[c + 7] == 'f')) {

                ignore_until_newline = true;
                if (ifdefs.top() == IfDefMode::POST_KEEP) {
                    continue;
                }
                if (ifdefs.top() == IfDefMode::KEEP) {
                    ifdefs.pop();
                    ifdefs.push(IfDefMode::POST_KEEP);
                    continue;
                }

                int name_start = -1;
                int name_end = -1;
                for (int cc = c + 8; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = IsWhiteSpace(curr);
                    if ((name_start >= 0) && (name_end < 0) && whitespace) {
                        name_end = cc;
                        break;
                    }
                    else if ((name_start < 0) && !whitespace) {
                        name_start = cc;
                    }
                }
                if (name_end >= 0) {
                    auto name = input.mid(name_start, name_end - name_start);
                    for (auto& replace : replacements) {
                        if (replace.p_Word == name) {
                            ifdefs.pop();
                            ifdefs.push(IfDefMode::KEEP);
                            break;
                        }
                    }
                }
                continue;
            // #else
            } else if ((!ifdefs.empty()) && (remaining >= 5) && (input[c + 1] == 'e') && (input[c + 2] == 'l') && (input[c + 3] == 's') && (input[c + 4] == 'e')) {
                ignore_until_newline = true;
                if (ifdefs.top() == IfDefMode::POST_KEEP) {
                    continue;
                }
                auto new_mode = ifdefs.top() == IfDefMode::KEEP ? IfDefMode::POST_KEEP : IfDefMode::KEEP;
                ifdefs.pop();
                ifdefs.push(new_mode);
                continue;
            // #endif
            } else if ((!ifdefs.empty()) && (remaining >= 6) && (input[c + 1] == 'e') && (input[c + 2] == 'n') && (input[c + 3] == 'd') && (input[c + 4] == 'i') && (input[c + 5] == 'f')) {
                ifdefs.pop();
                ignore_until_newline = true;
                continue;
            // #include
            } else if ((!remove_code) && (remaining >= 8) && (input[c + 1] == 'i') && (input[c + 2] == 'n') && (input[c + 3] == 'c') && (input[c + 4] == 'l') && (input[c + 5] == 'u') && (input[c + 6] == 'd') && (input[c + 7] == 'e')) {

                int fname_start = -1;
                int fname_end = -1;
                for (int cc = c + 8; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool is_quote = curr == '"';
                    bool endline = (curr == '\n') || (curr == '\r');
                    if (endline) {
                        break;
                    } else if ((fname_start >= 0) && (fname_end < 0) && is_quote) {
                        fname_end = cc;
                        break;
                    } else if ((fname_start < 0) && is_quote) {
                        fname_start = cc + 1;
                    }
                }
                ignore_until_newline = true;
                if (fname_end >= 0) {
                    auto fname = input.mid(fname_start, fname_end - fname_start);

                    if (includes.find(fname) != includes.end()) {
                        continue;
                    }
                    includes.insert(fname);

                    std::string error;
                    std::string fname_str(fname.data(), fname.size());
                    auto included_data = loader(fname_str, error);
                    if (!included_data.isNull()) {
                        // modifies local copy of input so preprocessor can process args as well
                        input.replace(c, fname_end - c + 1, included_data);
                        ignore_until_newline = false;
                        c--;
                        continue;
                    }
                    output += QString("#error file \"%1\" not found").arg(QString(fname)).toLocal8Bit();
                    ignore_until_newline = false;
                    c--;
                    continue;
                }
            }

        } else if ((!remove_code) && (!multi_line_comment) && (!line_comment)) {

            bool found = false;
            for (const auto& replace : replacements) {
                bool match = true;
                for (int i = 0; i < replace.p_Word.size(); i++) {
                    if (replace.p_Word[i] != input[c + i]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    if (replace.p_NumArgs > 0) {
                        int parenthesis = 0;
                        QByteArrayList args;
                        int arg_start = -1;
                        int arg_end = -1;
                        for (int cc = c + replace.p_Word.size(); cc < input.size(); cc++) {
                            char curr = input[cc];
                            if (curr == '(') {
                                parenthesis++;
                            } else if (curr == ')') {
                                parenthesis--;
                                if (parenthesis == 0) {
                                    if ((arg_start >= 0) && (arg_end >= 0)) {
                                        args += input.mid(arg_start, arg_end - arg_start);
                                    }
                                    arg_end = cc;
                                    break;
                                }
                                arg_end = cc + 1;
                            } else if (curr == ',') {
                                if ((arg_start >= 0) && (arg_end >= 0)) {
                                    args += input.mid(arg_start, arg_end - arg_start);
                                }
                                arg_start = -1;
                                arg_end = -1;
                            } else if (curr != ' ') {
                                if (arg_start < 0) {
                                    arg_start = cc;
                                }
                                arg_end = cc + 1;
                            }
                        }
                        if (replace.p_NumArgs != args.size()) {
                            err_msg = "Incorrect number of args for macro " + replace.p_Word;
                            return QByteArray();
                        }
                        QByteArray replacement;
                        for (const auto& piece : replace.p_FunctionPieces) {
                            if (std::holds_alternative<int>(piece)) {
                                replacement += args[std::get<int>(piece)];
                            } else {
                                replacement += std::get<QByteArray>(piece);
                            }
                        }
                        // modifies local copy of input so preprocessor can process args as well
                        input.replace(c, arg_end - c + 1, replacement);
                        c--;
                    } else if (!replace.p_Replace.isNull()) {
                        output += replace.p_Replace;
                        c += replace.p_Word.size() - 1;
                    }
                    found = true;
                    break;
                }
            }
            if (found) {
                continue;
            }
        }
        if (remove_code) {
            continue;
        }
        output += val;
        last_val = val;
    }

    return output;
}

}