#include <iostream>
#include <fstream>
#include <codecvt>
#include <unordered_set>
#include "aes.h"
#include "questions_answers.h"
#include <fcntl.h>

using namespace std;

TestContent set_answers_def_params() {
    SingleLevelContent open_section_content = SingleLevelContent(L". \"Текст ответа\"\n", 3);
    vector<size_t> twolevel_contents_num_rows = { 1, 1, 2 };
    TwoLevelContent close_section_content = TwoLevelContent(L".\n", twolevel_contents_num_rows);
    return TestContent(L"Правильные ответы.\n", L"По закрытым вопросам.\n", L"По открытым вопросам.\n", open_section_content, close_section_content);
};

TestContent set_questions_def_params() {
    return TestContent();
};

void test_aes() {
    AES::test_ECB();
    printf("\n");
    AES::test_CBC();
    printf("\n");
    AES::test_CFB();
}

void test_questions_answers() {
    wstring test_name;
    vector<vector<size_t>> twolevel_row_nums;
    vector<wstring> questions;
    vector<wstring> open_questions;
    vector<vector<wstring>> answers;
    bool res;
    size_t char_num;
    TestContent test_content_questions = set_questions_def_params();
    TestContent test_content_answers = set_answers_def_params();
    char_num = 0;
    res = SingleLevelContent::get_rows_content(test_content_questions.open_section_content.pattern + L"-", questions, char_num);
    if (res) wprintf(L"Шаблонная строка open_section_content класса вопросов соответствует шаблону!\n");
    else wprintf(L"Шаблонная строка open_section_content класса вопросов НЕ соответствует шаблону!\n");

    char_num = 0;
    res = TwoLevelContent::get_rows_content(test_content_questions.close_section_content.pattern + L"-", questions, answers, char_num);
    if (res) wprintf(L"Шаблонная строка close_section_content класса вопросов соответствует шаблону!\n");
    else wprintf(L"Шаблонная строка close_section_content класса вопросов НЕ соответствует шаблону!\n");

    char_num = 0;
    res = TwoLevelContent::get_row_nums(test_content_answers.close_section_content.pattern + L"-", twolevel_row_nums, char_num);
    if (res) wprintf(L"Шаблонная строка close_section_content класса ответов соответствует шаблону!\n");
    else wprintf(L"Шаблонная строка close_section_content класса ответов НЕ соответствует шаблону!\n");

    res = test_content_questions.get_content(test_content_questions.pattern, open_questions, questions, answers, test_name, twolevel_row_nums);
    if (res) wprintf(L"Полная шаблонная строка класса вопросов соответствует шаблону!\n");
    else wprintf(L"Полная шаблонная строка класса вопросов НЕ соответствует шаблону!\n");

    res = test_content_answers.get_content(test_content_answers.pattern, open_questions, questions, answers, test_name, twolevel_row_nums);
    if (res) wprintf(L"Полная шаблонная строка класса ответов соответствует шаблону!\n");
    else wprintf(L"Полная шаблонная строка класса ответов НЕ соответствует шаблону!\n");
};

bool read_from_file_wstr(wstring& file_name, wstring& fileContents) {
    wifstream file(file_name);
    if (!file.is_open()) {
        wprintf(L"- Не удалось открыть файл!\n");
        return false;
    }
    file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));
    fileContents = L"";
    wstring line;
    while (getline(file, line)) {
        fileContents += line + L"\n";
    }
    file.close();
    return true;
}

bool read_from_file_str(wstring& file_name, vector<unsigned char>& fileContents) {
    ifstream file(file_name, ios::binary);
    if (!file.is_open()) {
        wprintf(L"- Не удалось открыть файл!\n");
        return false;
    }
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);
    fileContents.resize(fileSize);
    file.read(reinterpret_cast<char*>(fileContents.data()), fileSize);
    file.close();
    return true;
}

bool write_to_file(wstring& file_name, vector<unsigned char>& text) {
    ofstream file(file_name, ios::binary);
    if (!file.is_open()) {
        wprintf(L"- Не удалось открыть файл!\n");
        return false;
    }
    file.write(reinterpret_cast<const char*>(text.data()), text.size());
    file.close();
    return true;
}

vector<unsigned char> encrypt(wstring content) {
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    string utf8str = converter.to_bytes(content);
    size_t text_size = utf8str.size() + 1;
    size_t buffer_size = (size_t)(16 * ceil(text_size / 16.0));
    uint8_t* buffer = new uint8_t[buffer_size];
    copy(utf8str.begin(), utf8str.end(), buffer);
    buffer[text_size - 1] = '\0';
    for (size_t i = text_size; i < buffer_size; i++)
        buffer[i] = 0x00;
    uint8_t key[Nk * Nwb] =
    {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    uint8_t iv[Nstb] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    AES::CFB_encrypt(key, iv, buffer, buffer_size);
    vector<unsigned char> enc_buffer(buffer, buffer + buffer_size);
    delete[] buffer;
    return enc_buffer;
}

void decrypt(vector<unsigned char> content, wstring& res) {
    size_t buffer_size = content.size();
    uint8_t* buffer = new uint8_t[buffer_size];
    copy(content.begin(), content.end(), buffer);
    uint8_t key[Nk * Nwb] =
    {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    uint8_t iv[Nstb] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    AES::CFB_decrypt(key, iv, buffer, buffer_size);
    size_t text_size = buffer_size - 1;
    while (text_size >= 0) {
        if (buffer[text_size] == '\0') {
            break;
        }
        --text_size;
    }
    std::string str_output((char*)(buffer), text_size);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    try {
        res = converter.from_bytes(str_output);
    }
    catch (const std::range_error& e) {
        wcerr << L"Не удалось преобразовать string в wstring UTF-8: " << e.what() << endl;
    }
}

bool get_answers(wstring str, unordered_set<size_t>& answers, size_t max_answer_num) {
    answers = unordered_set<size_t>();
    wstring num = L"";
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] != L',') num += str[i];
        if (str[i] == L',' || i == str.size() - 1) {
            if (!isNumber(num)) return false;
            size_t ans_num = stoi(num);
            if (ans_num > max_answer_num) return false;
            answers.insert(ans_num);
            num = L"";
        }
    }
    return true;
}

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    bool test_AES_flag = false;
    bool test_questions_answers_flag = false;
    if (test_questions_answers_flag) {
        test_questions_answers();
        return 0;
    }
    if (test_AES_flag) {
        test_aes();
        return 0;
    }
    wstring file_pattern_warning = L"- Содержимое файла должно соответствовать шаблону!\n";
    wstring file_pattern_success = L"- Выбранный файл соответствует шаблону!\n";
    wstring prog_mode = L"- Выберите режим работы программы:\n"
        "1. Разработка теста\n"
        "2. Тестирование";
    int mode;
    TestContent test_content_questions = set_questions_def_params();
    TestContent test_content_answers = set_answers_def_params();
    wstring test_name;
    vector<vector<size_t>> twolevel_row_nums;
    vector<wstring> questions;
    vector<wstring> open_questions;
    vector<vector<wstring>> answers;
    wstring file_name, file_content;
    vector<unsigned char> encrypted_content;
    for (size_t i = 0; i < 100; ++i) {
        wprintf(L"%s\n- ", prog_mode.c_str());
        wcin >> mode;
        wcin.clear();
        wcin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (mode == 1 || mode == 2) break;
    }
    if (mode == 1) {
        wstring welcome_message =
            L"- Для создания теста необходимо составить и затем выбрать 2 файла заданного шаблона:\n"
            "1. Файл с вопросами и вариантами ответов;\n"
            "2. Файл с правильными ответами.\n";
        wstring requirements_for_creating_a_test =
            L"- Требования шаблона:\n"
            "1. На каждый закрытый вопрос (с выбором вариантов ответов из возможных) кол-во \n"
            "возможных ответов должно быть больше кол-ва правильных ответов;\n"
            "2. Минимум один правильный ответ на каждый вопрос;\n"
            "3. На каждый открытый вопрос должен быть только один правильный ответ.\n"
            "4. Вместо текста внутри кавычек вводится конкретное содержимое. Кавычки можно опустить.\n";
        wstring file_patterns = L"- Шаблон файла с вопросами и вариантами ответов:\n" +
            test_content_questions.pattern +
            L"\n- Шаблон файла с правильными ответами:\n" +
            test_content_answers.pattern;
        wprintf(L"%s\n%s\n%s\n", welcome_message.c_str(), requirements_for_creating_a_test.c_str(), file_patterns.c_str());
        for (size_t j = 0; j < 100; ++j) {
            wprintf(L"- Введите имя файла с вопросами и вариантами ответов:\n- ");
            getline(wcin >> ws, file_name);
            if (read_from_file_wstr(file_name, file_content)) break;
        }
        if (!test_content_questions.get_content(file_content, open_questions, questions, answers, test_name, twolevel_row_nums)) {
            wprintf(L"%s", file_pattern_warning.c_str());
            return 0;
        }
        wprintf(L"%s", file_pattern_success.c_str());
        for (size_t j = 0; j < 100; ++j) {
            wprintf(L"- Введите имя файла с правильными ответами для его шифрования:\n- ");
            getline(wcin >> ws, file_name);
            if (read_from_file_wstr(file_name, file_content)) break;
        }
        if (!test_content_answers.get_content(file_content, open_questions, questions, answers, test_name, twolevel_row_nums)) {
            wprintf(L"%s", file_pattern_warning.c_str());
            return 0;
        }
        wprintf(L"%s", file_pattern_success.c_str());
        encrypted_content = encrypt(file_content);
        wprintf(L"- Содержимое файла с правильными ответами успешно зашифровано.\n");
        for (size_t j = 0; j < 100; ++j) {
            wprintf(L"- Введите имя файла для записи зашифрованного текста:\n- ");
            getline(wcin >> ws, file_name);
            if (write_to_file(file_name, encrypted_content)) break;
        }
        wprintf(L"- Зашифрованное содержимое успешно записано в файл.\n");
    }
    else {
        for (size_t j = 0; j < 100; ++j) {
            wprintf(L"- Введите имя файла с вопросами и вариантами ответов:\n- ");
            getline(wcin >> ws, file_name);
            if (read_from_file_wstr(file_name, file_content)) break;
        }
        if (!test_content_questions.get_content(file_content, open_questions, questions, answers, test_name, twolevel_row_nums)) {
            wprintf(L"%s", file_pattern_warning.c_str());
            return 0;
        }
        wprintf(L"%s", file_pattern_success.c_str());
        for (size_t j = 0; j < 100; ++j) {
            wprintf(L"- Введите имя зашифрованного файла с правильными ответами:\n- ");
            getline(wcin >> ws, file_name);
            if (read_from_file_str(file_name, encrypted_content)) break;
        }
        vector<wstring> open_answers;
        wstring decrypted_content;
        decrypt(encrypted_content, decrypted_content);
        if (!test_content_answers.get_content(decrypted_content, open_answers, questions, answers, test_name, twolevel_row_nums)) {
            wprintf(L"%s", file_pattern_warning.c_str());
            return 0;
        }
        wprintf(L"%s", file_pattern_success.c_str());
        wstring testing = L"\n- Тестирование:\n"
            "Для ответа на закрытые вопросы в случае если вы хотите отметить\n"
            "несколько ответов вводите их номера через запятую, например: 2,3\n";
        wstring user_answer;
        vector<unordered_set<size_t>> user_answers;
        vector<wstring> user_open_answers;
        wprintf(L"%s", testing.c_str());
        wprintf(L"- Закрытые вопросы: \n");
        for (size_t j = 0; j < questions.size(); ++j) {
            user_answers.push_back(unordered_set<size_t>());
            wprintf(L"%zi. %s\n", j + 1, questions[j].c_str());
            for (size_t k = 0; k < answers[j].size(); ++k) {
                wprintf(L"%zi. %s\n", k + 1, answers[j][k].c_str());
            }
            for (size_t k = 0; k < 100; ++k) {
                wprintf(L"- Выберите правильные ответы:\n- ");
                getline(wcin >> ws, user_answer);
                if (get_answers(user_answer, user_answers[j], answers[j].size())) break;
            }
        }
        wprintf(L"- Открытые вопросы: \n");
        for (size_t j = 0; j < open_questions.size(); ++j) {
            wprintf(L"%zi. %s\n", j + 1, open_questions[j].c_str());
            wprintf(L"- Введите ответ:\n- ");
            getline(wcin >> ws, user_answer);
            user_open_answers.push_back(user_answer);
        }
        size_t num_user_correct_answers = 0;
        size_t num_correct_answers = 0;
        for (size_t j = 0; j < twolevel_row_nums.size(); ++j) {
            num_correct_answers += twolevel_row_nums[j].size();
            for (size_t k = 0; k < twolevel_row_nums[j].size(); ++k) {
                auto iter = user_answers[j].find(twolevel_row_nums[j][k]);
                if (iter != user_answers[j].end()) ++num_user_correct_answers;
            }
        }
        num_correct_answers += open_answers.size();
        for (size_t j = 0; j < open_answers.size(); ++j) {
            if ((open_answers[j] == L"\"" + user_open_answers[j] + L"\"") || (open_answers[j] == user_open_answers[j])) ++num_user_correct_answers;
        }
        double num_points = ((double)num_user_correct_answers / num_correct_answers) * 100.0;
        wprintf(L"\n- Результат:\n");
        wprintf(L"Количество правильных ответов: %zi из %zi (%.2lf%% из 100%%)!\n",
            num_user_correct_answers, num_correct_answers, num_points);
    }
    return 0;
}
