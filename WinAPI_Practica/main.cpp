#include <windows.h>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <locale>     // для imbue
#include <io.h>
#include <fcntl.h>
// Класс для замены точки на запятую в десятичной части
struct comma_decimal : std::numpunct<wchar_t> {
protected:
    wchar_t do_decimal_point() const override { return L','; }
    wchar_t do_thousands_sep() const override { return L'\0'; }
    std::string do_grouping() const override { return ""; }
};

static HWND hwndCount, hwndMin, hwndMax, hwndUnique, hwndInputFile, hwndReadFile, hwndOutputFile; // поля ввода
static std::vector<int> vec, before;


#define ID_EDIT_COUNT 2001
#define ID_EDIT_MIN 2002
#define ID_EDIT_MAX 2003
#define ID_CHECKBOX_UNIQUE 3001
#define ID_EDIT_INPUT_FILE 2004
#define ID_EDIT_READ_FILE 2005
#define ID_EDIT_OUTPUT_FILE 2006
#define ID_BUTTON_GENERATE_WRITE 1004
#define ID_BUTTON_READ_SORT 1005
#define ID_BUTTON_EXIT 1006

std::wstring GetEditText(HWND hwndEdit) {
    int len = GetWindowTextLengthW(hwndEdit) + 1;
    std::vector<wchar_t> buffer(len);
    GetWindowTextW(hwndEdit, buffer.data(), len);
    return std::wstring(buffer.data());
}

bool validInt(std::wstring& str, bool canBeNegative = false) {
    str.erase(0, str.find_first_not_of(L" "));
    str.erase(str.find_last_not_of(L" ") + 1);
    if (str.empty()) return false;
    if (!canBeNegative && str[0] == L'-') return false;
    for (wchar_t c : str) {
        if (c != L'-' && (c < L'0' || c > L'9')) return false;
    }
    return true;
}

bool validFilename(std::wstring& str) {
    str.erase(0, str.find_first_not_of(L" "));
    str.erase(str.find_last_not_of(L" ") + 1);
    if (str.empty()) return false;
    std::wstring forbidden = L"<>:\"/\\|?*";
    for (wchar_t c : str) {
        if (forbidden.find(c) != std::wstring::npos) return false;
    }
    return true;
}

void ShowVec(const std::vector<int>& v) {
    for (int num : v) {
        std::wcout << num << L" ";
    }
    std::wcout << std::endl;
}

bool GenerateUniqueVector(std::vector<int>& vec, int n) {
    if (n <= 0) {
        MessageBoxW(NULL, L"Количество элементов должно быть больше 0!", L"Ошибка", MB_OK | MB_ICONERROR);
        return false;
    }
    vec.clear();
    for (int i = 0; i < n; i++) {
        vec.push_back(i);
    }
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(vec[i], vec[j]);
    }
    return true;
}

bool WriteVectorToFile(const std::vector<int>& vec, const std::wstring& filename, bool isOutputFile = false) {
    std::wofstream out(filename);
    out.imbue(std::locale(".1251")); 

    if (!out.is_open()) return false;

    if (isOutputFile) {
        time_t now = time(0);
        char timeStr[26];
        ctime_s(timeStr, sizeof(timeStr), &now);
        std::wstring wTimeStr;
        for (int i = 0; timeStr[i]; i++) {
            wTimeStr += static_cast<wchar_t>(timeStr[i]);
        }
        out << wTimeStr.substr(0, wTimeStr.length() - 1) << L"\n";
        out << L"результат сортировки: ";
        for (size_t i = 0; i < vec.size(); i++) {
            out << vec[i];
            if (i < vec.size() - 1) out << L", ";
        }
        out << std::endl;
    }
    else {
        for (size_t i = 0; i < vec.size(); i++) {
            out << vec[i];
            if (i < vec.size() - 1) out << L" ";
        }
        out << std::endl;
    }

    out.close();
    return true;
}

bool ReadVectorFromFile(std::vector<int>& vec, const std::wstring& filename) {
    vec.clear();
    std::wifstream in(filename);
    in.imbue(std::locale(".1251")); // чтобы корректно читались кириллица/числа
    if (!in.is_open()) return false;
    int num;
    while (in >> num) {
        vec.push_back(num);
    }
    in.close();
    return !vec.empty();
}

void QuickSort(std::vector<int>& vec, int left, int right, int& swaps) {
    if (left >= right) return;

    int pivot = vec[(left + right) / 2];
    int i = left;
    int j = right;

    while (i <= j) {
        while (vec[i] < pivot) i++;
        while (vec[j] > pivot) j--;

        if (i <= j) {
            std::swap(vec[i], vec[j]);
            swaps++;
            i++;
            j--;
        }
    }
    if (left < j) QuickSort(vec, left, j, swaps);
    if (i < right) QuickSort(vec, i, right, swaps);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int count = 0;
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Количество элементов:", WS_VISIBLE | WS_CHILD, 20, 20, 150, 20, hwnd, NULL, NULL, NULL);
        hwndCount = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 20, 100, 20, hwnd, (HMENU)ID_EDIT_COUNT, NULL, NULL);
        CreateWindowW(L"STATIC", L"Нижняя граница:", WS_VISIBLE | WS_CHILD, 20, 50, 150, 20, hwnd, NULL, NULL, NULL);
        hwndMin = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 50, 100, 20, hwnd, (HMENU)ID_EDIT_MIN, NULL, NULL);
        CreateWindowW(L"STATIC", L"Верхняя граница:", WS_VISIBLE | WS_CHILD, 20, 80, 150, 20, hwnd, NULL, NULL, NULL);
        hwndMax = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 80, 100, 20, hwnd, (HMENU)ID_EDIT_MAX, NULL, NULL);
        hwndUnique = CreateWindowW(L"BUTTON", L"Уникальные элементы", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 110, 220, 20, hwnd, (HMENU)ID_CHECKBOX_UNIQUE, NULL, NULL);
        CreateWindowW(L"STATIC", L"Файл для записи:", WS_VISIBLE | WS_CHILD, 20, 140, 150, 20, hwnd, NULL, NULL, NULL);
        hwndInputFile = CreateWindowW(L"EDIT", L"input.txt", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 140, 100, 20, hwnd, (HMENU)ID_EDIT_INPUT_FILE, NULL, NULL);
        CreateWindowW(L"STATIC", L"Файл для чтения:", WS_VISIBLE | WS_CHILD, 20, 170, 150, 20, hwnd, NULL, NULL, NULL);
        hwndReadFile = CreateWindowW(L"EDIT", L"input.txt", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 170, 100, 20, hwnd, (HMENU)ID_EDIT_READ_FILE, NULL, NULL);
        CreateWindowW(L"STATIC", L"Файл для результата:", WS_VISIBLE | WS_CHILD, 20, 200, 150, 20, hwnd, NULL, NULL, NULL);
        hwndOutputFile = CreateWindowW(L"EDIT", L"sorted.txt", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 200, 100, 20, hwnd, (HMENU)ID_EDIT_OUTPUT_FILE, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Генерировать и записать", WS_VISIBLE | WS_CHILD, 20, 230, 260, 30, hwnd, (HMENU)ID_BUTTON_GENERATE_WRITE, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Прочитать и отсортировать", WS_VISIBLE | WS_CHILD, 20, 270, 260, 30, hwnd, (HMENU)ID_BUTTON_READ_SORT, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Выход", WS_VISIBLE | WS_CHILD, 20, 310, 260, 30, hwnd, (HMENU)ID_BUTTON_EXIT, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON_GENERATE_WRITE: {
            std::wstring nStr = GetEditText(hwndCount);
            std::wstring minStr = GetEditText(hwndMin);
            std::wstring maxStr = GetEditText(hwndMax);
            if (!validInt(nStr) || !validInt(minStr, true) || !validInt(maxStr, true)) {
                MessageBoxW(hwnd, L"Некорректный ввод чисел!", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }
            int n = std::stoi(nStr);
            int MIN = std::stoi(minStr);
            int MAX = std::stoi(maxStr);
            if (n <= 0 || MAX <= MIN) {
                MessageBoxW(hwnd, L"Количество > 0, MAX > MIN", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }
            std::wstring inputFile = GetEditText(hwndInputFile);
            if (!validFilename(inputFile)) {
                MessageBoxW(hwnd, L"Неверное имя файла!", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }
            vec.clear();
            bool isUnique = (SendMessage(hwndUnique, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (isUnique) {
                if (!GenerateUniqueVector(vec, n)) break;
            }
            else {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(MIN, MAX);
                for (int i = 0; i < n; i++) {
                    vec.push_back(dist(gen));
                }
            }
            before = vec;
            if (WriteVectorToFile(vec, inputFile)) {
                std::wcout << L"Массив записан в файл " << inputFile << std::endl;
            }
            else {
                MessageBoxW(hwnd, L"Ошибка записи в файл!", L"Ошибка", MB_OK | MB_ICONERROR);
            }
            break;
        }

        case ID_BUTTON_READ_SORT: {
            std::wstring readFile = GetEditText(hwndReadFile);
            std::wstring outputFile = GetEditText(hwndOutputFile);
            if (!validFilename(readFile) || !validFilename(outputFile)) {
                MessageBoxW(hwnd, L"Проверьте имена файлов!", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }
            vec.clear();
            if (!ReadVectorFromFile(vec, readFile)) {
                MessageBoxW(hwnd, L"Не удалось прочитать файл!", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            auto start = std::chrono::high_resolution_clock::now();
            QuickSort(vec, 0, vec.size() - 1, count);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;

            if (WriteVectorToFile(vec, outputFile, true)) {
                std::wcout << L"Отсортированный массив записан в " << outputFile << std::endl;
            }
            else {
                MessageBoxW(hwnd, L"Ошибка записи в файл результата!", L"Ошибка", MB_OK | MB_ICONERROR);
            }

            std::wstringstream ss;
            ss.imbue(std::locale(std::locale::classic(), new comma_decimal));
            ss << std::fixed << std::setprecision(3) << (duration.count() / 1000.0); // в секундах
            ss << L" сек\nКоличество перестановок: " << count;


            MessageBoxW(hwnd, ss.str().c_str(), L"Результаты", MB_OK | MB_ICONINFORMATION);
            break;
        }

        case ID_BUTTON_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    srand(static_cast<unsigned>(time(0)));
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    //std::wcout.imbue(std::locale("ru_RU.UTF-8"));

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SortApp";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"SortApp", L"Сортировка массива",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 400,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
