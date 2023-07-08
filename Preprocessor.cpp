////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Neshny {

/*
supported:
    #define
    #undef
will support:
    #include
    #ifdef
    #ifndef
    #else
    #elifdef - else defined
    #elifndef - else not defined
    #endif
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
    };

    std::list<ReplaceWords> replacements = {};

    bool multi_line_comment = false;
    bool line_comment = false;
    char last_val = 0;
    for (int c = 0; c < input.size(); c++) {
        char val = input[c];
        if ((!multi_line_comment) && (!line_comment) && (val == '*') && (last_val == '/')) {
            multi_line_comment = true;
        } else if (multi_line_comment && (val == '/') && (last_val == '*')) {
            multi_line_comment = false;
        } else if ((!multi_line_comment) && (val == '/') && (last_val == '/')) {
            line_comment = true;
        } else if (val == '\n') {
            line_comment = false;
        } else if ((!multi_line_comment) && (!line_comment) && (val == '#')) {

            int remaining = input.size() - c;
            // #define
            if ((remaining > 7) && (input[c + 1] == 'd') && (input[c + 2] == 'e') && (input[c + 3] == 'f') && (input[c + 4] == 'i') && (input[c + 5] == 'n') && (input[c + 6] == 'e')) {
                
                int name_start = -1;
                int name_end = -1;
                int def_start = -1;
                int def_end = -1;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = (curr == ' ') || (curr == '\n');
                    if ((def_start >= 0) && whitespace) {
                        def_end = cc;
                        break;
                    } else if ((name_end >= 0) && (def_start < 0) && !whitespace) {
                        def_start = cc;
                    } else if ((name_start >= 0) && (name_end < 0) && whitespace) {
                        name_end = cc;
                    } else if((name_start < 0) && !whitespace) {
                        name_start = cc;
                    }
                }
                if (def_end >= 0) {
                    replacements.push_back({
                        input.mid(name_start, name_end - name_start),
                        input.mid(def_start, def_end - def_start)
                    });
                    c = def_end - 1;
                    continue;
                }
            // #undef
            } else if ((remaining > 6) && (input[c + 1] == 'u') && (input[c + 2] == 'n') && (input[c + 3] == 'd') && (input[c + 4] == 'e') && (input[c + 5] == 'f')) {

                int name_start = -1;
                int name_end = -1;
                for (int cc = c + 7; cc < input.size(); cc++) {
                    char curr = input[cc];
                    bool whitespace = (curr == ' ') || (curr == '\n');
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
                            break;
                        }
                    }
                    if (removed) {
                        continue;
                    }
                }
            }

        } else if ((!multi_line_comment) && (!line_comment)) {

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
                    output += replace.p_Replace;
                    c += replace.p_Word.size() - 1;
                    found = true;
                    break;
                }
            }
            if (found) {
                continue;
            }

        }
        output += val;
        last_val = val;
    }

    return output;
}

}