////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Neshny {

/*
supported:
    #define A B
    #define A(x) (x * x)
    #undef
    #ifdef
    #else
    #endif
will support:
    #include
    #ifndef
    #elifdef - else defined
    #elifndef - else not defined
not supported:
    #if
*/

////////////////////////////////////////////////////////////////////////////////
QByteArray Preprocess(QByteArray input, const std::function<QByteArray(QString, QString&)>& loader, QString& err_msg) {

    QByteArray output;
    output.reserve(input.size());

    struct ReplaceWords {
        QByteArray  p_Word;
        QByteArray  p_Replace;
        int         p_NumArgs;
        std::vector<std::variant<QByteArray, int>> p_FunctionPieces;
    };

    enum class IfDefMode {
        KEEP,
        REMOVE,
        ELSE_KEEP,
        ELSE_REMOVE
    };

    std::list<ReplaceWords> replacements = {};

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
        bool remove_code = ((!ifdefs.empty()) && ((ifdefs.top() == IfDefMode::REMOVE) || (ifdefs.top() == IfDefMode::ELSE_REMOVE)));


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
            if ((!remove_code) && (remaining > 7) && (input[c + 1] == 'd') && (input[c + 2] == 'e') && (input[c + 3] == 'f') && (input[c + 4] == 'i') && (input[c + 5] == 'n') && (input[c + 6] == 'e')) {
                int name_start = -1;
                int name_end = -1;
                int def_start = -1;
                int def_end = -1;
                QByteArrayList arg_names;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool newline = curr == '\n';
                    bool whitespace = (curr == ' ') || newline || (curr == '\r');
                    if (newline) {
                        if ((def_end < 0) && (def_start >= 0)) {
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
                    replacements.push_back({
                        input.mid(name_start, name_end - name_start),
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
                    continue;
                }
            // #undef
            } else if ((!remove_code) && (remaining > 6) && (input[c + 1] == 'u') && (input[c + 2] == 'n') && (input[c + 3] == 'd') && (input[c + 4] == 'e') && (input[c + 5] == 'f')) {

                int name_start = -1;
                int name_end = -1;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = (curr == ' ') || (curr == '\n') || (curr == '\r');
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
            } else if ((!remove_code) && (remaining > 6) && (input[c + 1] == 'i') && (input[c + 2] == 'f') && (input[c + 3] == 'd') && (input[c + 4] == 'e') && (input[c + 5] == 'f')) {

                int name_start = -1;
                int name_end = -1;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = (curr == ' ') || (curr == '\n') || (curr == '\r');
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
                    ifdefs.push(found ? IfDefMode::KEEP : IfDefMode::REMOVE);
                }
                ignore_until_newline = true;
                continue;
            // #else
            } else if ((!ifdefs.empty()) && (remaining > 5) && (input[c + 1] == 'e') && (input[c + 2] == 'l') && (input[c + 3] == 's') && (input[c + 4] == 'e')) {
                auto top = ifdefs.top();
                ifdefs.pop();
                ifdefs.push(top == IfDefMode::KEEP ? IfDefMode::ELSE_REMOVE : IfDefMode::ELSE_KEEP);
                ignore_until_newline = true;
                continue;
            // #endif
            } else if ((!ifdefs.empty()) && (remaining > 6) && (input[c + 1] == 'e') && (input[c + 2] == 'n') && (input[c + 3] == 'd') && (input[c + 4] == 'i') && (input[c + 5] == 'f')) {
                ifdefs.pop();
                ignore_until_newline = true;
                continue;
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
                    } else {
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