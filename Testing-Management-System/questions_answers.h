#include <string>
#include <vector>
#include <cwctype>
#include <io.h>

using namespace std;

bool isNumber(wstring& str);

class Content {
public:
    wstring get_row_pattern(size_t row_num, wstring row_content);
    size_t num_rows;
};

class SingleLevelContent : public Content {
public:
    void set_pattern();
    wstring text;
    wstring pattern;
    SingleLevelContent();
    SingleLevelContent(wstring text, size_t num_rows);
    static bool get_row_content(wstring str, wstring& row_content, size_t& row_num, size_t& char_num);
    static bool get_row_num(wstring str, size_t& row_num, size_t& char_num);
    static bool get_rows_content(wstring str, vector<wstring>& rows_content, size_t& char_num);
};

class TwoLevelContent : public SingleLevelContent {
private:
    void set_pattern();
    vector<SingleLevelContent> twolevel_contents;
public:
    TwoLevelContent();
    TwoLevelContent(wstring text);
    TwoLevelContent(wstring text, vector<size_t> twolevel_contents_num_rows);
    TwoLevelContent(wstring text, size_t num_rows, vector<SingleLevelContent> twolevel_contents);
    static bool get_rows_content(wstring str, vector<wstring>& rows_content, vector<vector<wstring>>& twolevel_rows_content, size_t& char_num);
    static bool get_row_nums(wstring str, vector<vector<size_t>>& twolevel_row_nums, size_t& char_num);
};

class TestContent {
private:
    wstring title, row_separator;
    wstring test_name_title, test_name;
    wstring close_section_title;
    wstring open_section_title;
    void set_pattern();
public:
    wstring pattern;
    SingleLevelContent open_section_content;
    TwoLevelContent close_section_content;
    TestContent();
    TestContent(wstring title, wstring close_section_title, wstring open_section_title,
        SingleLevelContent open_section_content, TwoLevelContent close_section_content);
    TestContent(wstring title, wstring row_separator, wstring test_name_title, wstring test_name, wstring close_section_title,
        wstring open_section_title, SingleLevelContent open_section_content, TwoLevelContent close_section_content);
    bool get_content(wstring str, vector<wstring>& open_questions, vector<wstring>& questions,
        vector<vector<wstring>>& answers, wstring& test_name, vector<vector<size_t>>& twolevel_row_nums);
};