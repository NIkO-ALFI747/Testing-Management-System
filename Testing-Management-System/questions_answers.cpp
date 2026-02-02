#include "questions_answers.h"
#include <fcntl.h>

bool isNumber(wstring& str)
{
    if (str.empty() || str[0] == L'0') return false;
    bool isNumber = true;
    for (wstring::const_iterator k = str.begin(); k != str.end(); ++k) {
        int ch = wctob(*k);
        isNumber = isNumber && isdigit(ch);
    }
    return isNumber;
}

wstring Content::get_row_pattern(size_t row_num, wstring row_content) {
    return to_wstring(row_num) + row_content;
};

void SingleLevelContent::set_pattern()
{
    pattern = L"";
    for (size_t i = 0; i < num_rows; ++i)
    {
        pattern += get_row_pattern(i + 1, text);
    }
}

SingleLevelContent::SingleLevelContent() {
    num_rows = 3;
    text = L". \"Текст вопроса\"\n";
    pattern = L"";
}

SingleLevelContent::SingleLevelContent(wstring text, size_t num_rows = 3)
{
    this->num_rows = num_rows;
    this->text = text;
    set_pattern();
}

bool SingleLevelContent::get_row_content(wstring str, wstring& row_content, size_t& row_num, size_t& char_num) {
    wstring row_num_str = L"";
    row_content = L"";
    for (; str[char_num] != L'.'; ++char_num) {
        if (char_num == str.length() - 2) return false;
        row_num_str += str[char_num];
    }
    ++char_num;
    if ((str[char_num++] != L' ') || (row_num_str != to_wstring(row_num))) return false;
    ++row_num;
    for (; str[char_num] != L'\n'; ++char_num) {
        if (char_num == str.length() - 1) return false;
        row_content += str[char_num];
    }
    return true;
};

bool SingleLevelContent::get_row_num(wstring str, size_t& row_num, size_t& char_num) {
    wstring row_num_str = L"";
    for (; str[char_num] != L'.'; ++char_num) {
        if (char_num == str.length() - 2) return false;
        row_num_str += str[char_num];
    }
    if (str[++char_num] != L'\n' || !isNumber(row_num_str)) return false;
    row_num = stoi(row_num_str);
    return true;
};

bool SingleLevelContent::get_rows_content(wstring str, vector<wstring>& rows_content, size_t& char_num) {
    rows_content = vector<wstring>();
    wstring row_content = L"";
    size_t row_num = 1;
    do {
        if (char_num == str.length() - 1) return false;
        if (!get_row_content(str, row_content, row_num, char_num)) return false;
        rows_content.push_back(row_content);
        ++char_num;
    } while ((str[char_num] != L'-') || (str[char_num - 1] != L'\n'));
    return true;
};

void TwoLevelContent::set_pattern()
{
    pattern = L"";
    for (size_t i = 0; i < num_rows; ++i)
    {
        pattern += get_row_pattern(i + 1, text);
        for (size_t j = 0; j < twolevel_contents[i].num_rows; j++) {
            pattern += get_row_pattern(i + 1, L"." + get_row_pattern(j + 1, twolevel_contents[i].text));
        }
    }
}

TwoLevelContent::TwoLevelContent()
{
    for (size_t i = 0; i < num_rows; ++i) {
        twolevel_contents.push_back(SingleLevelContent(L". \"Текст варианта ответа\"\n"));
    }
    set_pattern();
}

TwoLevelContent::TwoLevelContent(wstring text)
{
    this->text = text;
    for (size_t i = 0; i < num_rows; ++i) {
        twolevel_contents.push_back(SingleLevelContent(L".\n"));
    }
    set_pattern();
}

TwoLevelContent::TwoLevelContent(wstring text, vector<size_t> twolevel_contents_num_rows) {
    this->text = text;
    for (size_t i = 0; i < twolevel_contents_num_rows.size(); ++i) {
        twolevel_contents.push_back(SingleLevelContent(L".\n", twolevel_contents_num_rows[i]));
    }
    set_pattern();
}

TwoLevelContent::TwoLevelContent(wstring text, size_t num_rows, vector<SingleLevelContent> twolevel_contents)
{
    if (num_rows != twolevel_contents.size()) {
        printf("Количество строк первого уровня больше размера списка строк второго уровня на каждую строку первого уровня!");
        for (size_t i = 0; i < num_rows; ++i) {
            twolevel_contents.push_back(SingleLevelContent(L". \"Текст варианта ответа\"\n"));
        }
        set_pattern();
    }
    else {
        this->num_rows = num_rows;
        this->text = text;
        this->twolevel_contents = twolevel_contents;
        set_pattern();
    }
}

bool TwoLevelContent::get_rows_content(wstring str, vector<wstring>& rows_content, vector<vector<wstring>>& twolevel_rows_content, size_t& char_num) {
    rows_content = vector<wstring>();
    twolevel_rows_content = vector<vector<wstring>>();
    size_t row_num = 1, twolevel_row_num, first_row_char_num;
    wstring row_content = L"", row_num_str;
    do {
        if (char_num == str.length() - 1) return false;
        first_row_char_num = char_num;
        row_num_str = L"";
        for (; str[char_num] != L'.'; ++char_num) {
            if (char_num == str.length() - 2) return false;
            row_num_str += str[char_num];
        }
        ++char_num;
        if (row_num_str == to_wstring(row_num)) {
            if (!get_row_content(str, row_content, row_num, first_row_char_num)) return false;
            char_num = first_row_char_num;
            twolevel_row_num = 1;
            twolevel_rows_content.push_back(vector<wstring>());
            rows_content.push_back(row_content);
        }
        else if ((row_num_str == to_wstring(row_num - 1)) && (row_num > 1)) {
            if (!get_row_content(str, row_content, twolevel_row_num, char_num)) return false;
            twolevel_rows_content[row_num - 2].push_back(row_content);
        }
        else return false;
        ++char_num;
    } while ((str[char_num] != L'-') || (str[char_num - 1] != L'\n'));
    return true;
};

bool TwoLevelContent::get_row_nums(wstring str, vector<vector<size_t>>& twolevel_row_nums, size_t& char_num) {
    twolevel_row_nums = vector<vector<size_t>>();
    size_t row_num = 1, twolevel_row_num;
    wstring row_num_str;
    do {
        if (char_num == str.length() - 1) return false;
        row_num_str = L"";
        for (; str[char_num] != L'.'; ++char_num) {
            if (char_num == str.length() - 2) return false;
            row_num_str += str[char_num];
        }
        ++char_num;
        if ((row_num_str == to_wstring(row_num)) && (str[char_num] == L'\n')) {
            ++row_num;
            twolevel_row_nums.push_back(vector<size_t>());
        }
        else if ((row_num_str == to_wstring(row_num - 1)) && (row_num > 1)) {
            if (!get_row_num(str, twolevel_row_num, char_num)) return false;
            twolevel_row_nums[row_num - 2].push_back(twolevel_row_num);
        }
        else return false;
        ++char_num;
    } while ((str[char_num] != L'-') || (str[char_num - 1] != L'\n'));
    return true;
};

void TestContent::set_pattern()
{
    pattern =
        title +
        test_name_title + test_name + L"\n" +
        row_separator +
        close_section_title + close_section_content.pattern +
        row_separator +
        open_section_title + open_section_content.pattern +
        row_separator;
}

TestContent::TestContent() {
    title = L"Вопросы и варианты ответов.\n";
    row_separator = L"-------------------------------------------------------------------\n";
    test_name_title = L"Название теста: ";
    test_name = L"\"Текст названия теста\"";
    close_section_title = L"Закрытые вопросы и возможные ответы к ним:\n";
    open_section_title = L"Открытые вопросы:\n";
    open_section_content = SingleLevelContent();
    open_section_content.set_pattern();
    close_section_content = TwoLevelContent();
    set_pattern();
}

TestContent::TestContent(wstring title, wstring close_section_title, wstring open_section_title,
    SingleLevelContent open_section_content, TwoLevelContent close_section_content) {
    row_separator = L"-------------------------------------------------------------------\n";
    test_name_title = L"Название теста: ";
    test_name = L"\"Текст названия теста\"";
    this->title = title;
    this->close_section_title = close_section_title;
    this->open_section_title = open_section_title;
    this->close_section_content = close_section_content;
    this->open_section_content = open_section_content;
    this->open_section_content.set_pattern();
    set_pattern();
}

TestContent::TestContent(wstring title, wstring row_separator, wstring test_name_title, wstring test_name, wstring close_section_title,
    wstring open_section_title, SingleLevelContent open_section_content, TwoLevelContent close_section_content) {
    this->title = title;
    this->row_separator = row_separator;
    this->test_name_title = test_name_title;
    this->test_name = test_name;
    this->close_section_title = close_section_title;
    this->open_section_title = open_section_title;
    this->close_section_content = close_section_content;
    this->open_section_content = open_section_content;
    this->open_section_content.set_pattern();
    set_pattern();
}

bool TestContent::get_content(wstring str, vector<wstring>& open_questions, vector<wstring>& questions,
    vector<vector<wstring>>& answers, wstring& test_name, vector<vector<size_t>>& twolevel_row_nums) {
    wstring title = L"", test_name_title = L"", open_section_title = L"", open_section_content = L"", close_section_title = L"", close_section_content = L"";
    size_t row_num;
    size_t char_num = 0, temp_char_num;
    for (; str[char_num] != L'\n'; ++char_num) {
        title += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    title += L'\n';
    if (title != this->title) return false;
    for (; (str[char_num] != L' ') || (str[char_num - 1] != L':'); ++char_num) {
        test_name_title += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    test_name_title += L' ';
    if (test_name_title != this->test_name_title) return false;
    test_name = L"";
    for (; str[char_num] != L'\n'; ++char_num) {
        test_name += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    row_separator = L"";
    for (; str[char_num] != L'\n'; ++char_num) {
        row_separator += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    row_separator += L'\n';
    if (row_separator != this->row_separator) return false;
    for (; str[char_num] != L'\n'; ++char_num) {
        close_section_title += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    close_section_title += L'\n';
    if (close_section_title != this->close_section_title) return false;
    temp_char_num = char_num;
    if (!SingleLevelContent::get_row_num(str, row_num, temp_char_num)) {
        if (!TwoLevelContent::get_rows_content(str, questions, answers, char_num)) return false;
    }
    else {
        if (!TwoLevelContent::get_row_nums(str, twolevel_row_nums, char_num)) return false;
    }
    row_separator = L"";
    for (; str[char_num] != L'\n'; ++char_num) {
        row_separator += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    row_separator += L'\n';
    if (row_separator != this->row_separator) return false;
    for (; str[char_num] != L'\n'; ++char_num) {
        open_section_title += str[char_num];
        if (char_num == str.length() - 2) return false;
    }
    ++char_num;
    open_section_title += L'\n';
    if (open_section_title != this->open_section_title) return false;
    if (!SingleLevelContent::get_rows_content(str, open_questions, char_num)) return false;
    row_separator = L"";
    for (; str[char_num] != L'\n'; ++char_num) {
        row_separator += str[char_num];
        if (char_num == str.length() - 1) break;
    }
    ++char_num;
    row_separator += L'\n';
    if (row_separator != this->row_separator) return false;
    return true;
};