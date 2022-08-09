#include "Functions.h"

#include <cmath>
#include <locale>
#include <codecvt>
#include <bitset>
#include <sstream>
#include <fstream>
#include <algorithm>

// Number manipulation
int absv(int v)
{
    int mask = v >> 31;
    return (mask ^ v) - mask;
}

bool is_equal_f(float f1, float f2, float epsilon)
{
    return (fabsf(f1 - f2) <= epsilon);
}

bool is_equal_d(double d1, double d2, double epsilon)
{
    return (fabs(d1 - d2) <= epsilon);
}

float sin_norm(float f)
{
    return sin((f * Math::PI) - (Math::PI * 0.5f)) * 0.5f + 0.5f;
}

float cos_norm(float f)
{
    return cos((f - 1.0f) * Math::PI) * 0.5f + 0.5f;
}

float cos_norm_scaled(float f, float scale)
{
    float so2 = scale * 0.5f;
    return cos((f - 1.0f) * acosf(1.0f - 2.0f / scale)) * so2 + 1.0f - so2;
    //return cos((f * Math::PI - Math::PI) * scale) * 0.5f + 0.5f;
}

float safe_sqrt(float f)
{
    if (f >= 0.0f)
        return sqrt(f);
    else
        return -sqrt(-f);
}

// Vectors
std::pair<float, float> point_as_vectors(Pos2D<float> v1, Pos2D<float> v2, Pos2D<float> point)
{
    float a = cross_product(v2, point) / cross_product(v2, v1);
    float b = cross_product(v1, point) / cross_product(v1, v2);
    return std::pair<float, float>(a, b);
}

// Geometry
int distance_manh(Pos2D<int> pos1, Pos2D<int> pos2)
{
    return absv(pos1.x - pos2.x) + absv(pos1.y - pos2.y);
}

// Complex geometry
float direction(Pos2D<float> startPos, Pos2D<float> endPos)
{
    return 0.0f;
}

Pos2D<float> point_in_direction(Pos2D<float> originPos, float direction, float distance)
{
    Pos2D<float> vec = { distance, 0.0f };
    vec = point_rotated_by({ 0.0f, 0.0f }, vec, direction);
    originPos += vec;

    return originPos;

    // REEEEEEEEEEEEEEEEE this shit is useless
    if (is_equal_f(direction, Math::PI * 0.5f, 0.001f)) {
        originPos.y -= distance;
        return originPos;
    }
    else if (is_equal_f(direction, Math::PI * 1.0f, 0.001f)) {
        originPos.x -= distance;
        return originPos;
    }
    else if (is_equal_f(direction, Math::PI * 1.5f, 0.001f)) {
        originPos.y += distance;
        return originPos;
    }
    else if (is_equal_f(direction, Math::PI * 2.0f, 0.001f)) {
        originPos.x += distance;
        return originPos;
    }
    else {
        float x = distance / sqrt(1.0f + pow(tanf(direction), 2));
        float y = distance / sqrt(1.0f + pow(1.0f / tanf(direction), 2));
        //float x = 0.0f;
        //float y = 0.0f;

        if      (direction > Math::PI * 0.0f && direction < Math::PI * 0.5f) {
            // First Quarter
            originPos.x += x;
            originPos.y -= y;
            //return originPos;
        }
        else if (direction > Math::PI * 0.5f && direction < Math::PI * 1.0f) {
            // Second Quarter
            originPos.x -= x;
            originPos.y -= y;
            //return originPos;
        }
        else if (direction > Math::PI * 1.0f && direction < Math::PI * 1.5f) {
            // Third Quarter
            originPos.x -= x;
            originPos.y += y;
            //return originPos;
        }
        else if (direction > Math::PI * 1.5f && direction < Math::PI * 2.0f) {
            // Fourth Quarter
            originPos.x += x;
            originPos.y += y;
            //return originPos;
        }

        return originPos;
    }
}

Pos2D<float> point_rotated_by(Pos2D<float> rotationAxis, Pos2D<float> p, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    // Translate point back to origin
    p -= rotationAxis;

    // Totate point
    float xnew = p.x * c + p.y * s;
    float ynew = -p.x * s + p.y * c;

    // translate point back:
    p.x = xnew + rotationAxis.x;
    p.y = ynew + rotationAxis.y;

    return p;
}

// Strings

// Use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
std::string wstring_to_string(const std::wstring& ws)
{
    // Setup converter
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    return converter.to_bytes(ws);
}

std::wstring string_to_wstring(const std::string& s)
{
    // Setup converter
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    return converter.from_bytes(s);
}

void split_str(const std::string& str, std::vector<std::string>& output, char split, bool ignoreEmpty)
{
    int beginIndex = 0;
    while (beginIndex < str.length())
    {
        // Find start point
        if (ignoreEmpty)
        {
            while (str[beginIndex] == split)
                beginIndex++;
        }
        if (beginIndex == str.length())
            break;

        int i = beginIndex;
        for (; i < str.length(); i++)
        {
            if (str[i] == split)
            {
                output.push_back(str.substr(beginIndex, i - beginIndex));
                beginIndex = i + 1;
                break;
            }
        }
        if (i == str.length())
        {
            output.push_back(str.substr(beginIndex));
            break;
        }
    }
}

void split_wstr(const std::wstring& str, std::vector<std::wstring>& output, wchar_t split, bool ignoreEmpty)
{
    int beginIndex = 0;
    while (beginIndex < str.length())
    {
        // Find start point
        if (ignoreEmpty)
        {
            while (str[beginIndex] == split)
                beginIndex++;
        }
        if (beginIndex == str.length())
            break;

        int i = beginIndex;
        for (; i < str.length(); i++)
        {
            if (str[i] == split)
            {
                output.push_back(str.substr(beginIndex, i - beginIndex));
                beginIndex = i + 1;
                break;
            }
        }
        if (i == str.length())
        {
            output.push_back(str.substr(beginIndex));
            break;
        }
    }
}

int64_t str_to_int(const std::string& str, int base)
{
    bool negative = false;
    
    std::vector<int> nums;
    nums.reserve(9);

    // Get numbers from string
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == '-') {
            negative = true;
        }
        else {
            int val = str[i] - 48;

            if (val >= 0 && val <= 9) {
                nums.push_back(val);
            }
        }

        if (nums.size() > 8) continue;
    }
    if (nums.size() == 0) return 0;

    // Combine digits into a number
    int size = nums.size();
    int mult = 1;
    int fnum = 0;

    for (int i = size - 1; i >= 0; i--) {
        fnum += nums[i] * mult;
        mult *= base;
    }

    if (negative) fnum = -fnum;

    return fnum;
}

float str_to_float(const std::string& str)
{
    return std::stof(str);
}

double str_to_double(const std::string& str)
{
    return std::stod(str);
}

std::string int_to_str(int64_t num)
{
    std::ostringstream ss;
    ss << num;
    return ss.str();
}

std::string float_to_str(float num)
{
    std::ostringstream ss("");
    ss << num;
    return ss.str();
}

std::string double_to_str(double num)
{
    std::ostringstream ss("");
    ss << num;
    return ss.str();
}

std::string to_uppercase(const std::string& str)
{
    std::string newStr = str;
    for (int i = 0; i < newStr.size(); i++)
    {
        newStr[i] = toupper(newStr[i]);
    }
    return newStr;
}

std::string to_lowercase(const std::string& str)
{
    std::string newStr = str;
    for (int i = 0; i < newStr.size(); i++)
    {
        newStr[i] = tolower(newStr[i]);
    }
    return newStr;
}

int find_text_in_vec(const std::string& str, std::vector<std::string> vec)
{
    for (int i = 0; i < vec.size(); i++)
        if (vec[i] == str)
            return i;

    return -1;
}

int find_text_in_arr(const std::string& str, std::string arr[], int size)
{
    for (int i = 0; i < size; i++)
        if (arr[i] == str)
            return i;

    return -1;
}

std::string extract_str_until(const std::string& str, char tc)
{
    std::string s;
    s.reserve(10);

    for (unsigned i = 0; i < str.length(); i++) {
        if (str[i] != tc)
            s.push_back(str[i]);
        else
            break;
    }

    return s;
}

void erase_str_until(std::string &str, char tc)
{
    int l = extract_str_until(str, tc).length();

    str.erase(str.begin(), str.begin() + l);
    while (str[0] == tc) {
        str.erase(str[0]);
    }
}

std::string exer_str_until(std::string &str, char tc)
{
    std::string s = extract_str_until(str, tc);

    str.erase(str.begin(), str.begin() + s.length());
    while (str[0] == tc) {
        str.erase(str.begin(), str.begin() + 1);
    }

    return s;
}

// Bounds
bool add_bounds(Pos2D<int> &pos, const RECT &bounds)
{
    bool inBounds = true;
    // x
    if        (pos.x < bounds.left) {
        pos.x = bounds.left;
        inBounds = false;
    }
    else if (pos.x > bounds.right) {
        pos.x = bounds.right;
        inBounds = false;
    }
    // y
    if        (pos.y < bounds.top) {
        pos.y = bounds.top;
        inBounds = false;
    }
    else if (pos.y > bounds.bottom) {
        pos.y = bounds.bottom;
        inBounds = false;
    }

    return inBounds;
}

bool add_bounds(int &value, const int &min_v, const int &max_v)
{
    if        (value < min_v) {
        value = min_v;
        return false;
    }
    else if (value > max_v) {
        value = max_v;
        return false;
    }

    return true;
}

bool add_bounds(float &value, const float &min_v, const float &max_v)
{
    if (value < min_v) {
        value = min_v;
        return false;
    }
    else if (value > max_v) {
        value = max_v;
        return false;
    }

    return true;
}

bool check_bounds(const Pos2D<int> &pos, const RECT &bounds)
{
    // x
    if (pos.x < bounds.left)
        return false;
    if (pos.x > bounds.right)
        return false;
    // y
    if (pos.y < bounds.top)
        return false;
    if (pos.y > bounds.bottom)
        return false;

    return true;
}

bool check_bounds(const int &value, const int &min_v, const int &max_v)
{
    if (value < min_v)
        return false;
    if (value > max_v)
        return false;

    return true;
}

bool check_bounds(const float &value, const float &min_v, const float &max_v)
{
    if (value < min_v)
        return false;
    if (value > max_v)
        return false;

    return true;
}
