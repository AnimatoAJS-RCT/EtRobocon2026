#include "Util.h"

/*
    文字列を分割する

    注意
        s の文字列内に、区切り文字が連続する箇所があるとき、その隙間は無視される。
        たとえば、s が "a//b"、separator が "/" のとき、分割結果は、
        "a" と "b" であり、戻り値は 2 を返す。

    引数
        s: 対象の文字列。書き換えが起こる。NULL不可
        separator: 区切り文字の並び。NULL不可
        result: 結果を格納する文字列配列。十分な要素数を確保すること
        result_size: 引数result の要素数

    戻り値
        分割後の文字列の個数
*/
/*
size_t split(char* s, const char* separator, char** result, size_t result_size)
{
    assert(s != NULL);
    assert(separator != NULL);
    assert(result != NULL);
    assert(result_size > 0);
    size_t i = 0;

    // sがリテラルやconstの場合に備え、strtok用の一時バッファを用意
    char tmp[512];
    strncpy(tmp, s, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';
    char* p = strtok(tmp, separator);
    while(p != NULL) {
        assert(i < result_size);
        result[i] = p;
        ++i;

        p = strtok(NULL, separator);
    }

    return i;
}
*/


std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delimiter[0])) {
        result.push_back(item);
    }
    return result;
}

eColor getColor(int hue, int saturation, int value) {
    // 暗い領域を優先して黒判定する
    if(value < 32) {
        return BLACK;
    }

    // 彩度が低く一定以上明るい領域を白判定する
    if(saturation < 40 && value >= 32) {
        return WHITE;
    }

    // 色相ベースの色判定
    if(hue <= 20 || hue >= 340) {
        return RED;
    }
    if(hue >= 40 && hue <= 80) {
        return YELLOW;
    }
    if(hue >= 100 && hue <= 160) {
        return GREEN;
    }
    if(hue >= 200 && hue <= 280) {
        return BLUE;
    }

    return OTHER;
}

const char* colorToString(eColor color) {
    switch (color) {
        case RED:    return "RED";
        case BLUE:   return "BLUE";
        case YELLOW: return "YELLOW";
        case GREEN:  return "GREEN";
        case WHITE:  return "WHITE";
        case BLACK:  return "BLACK";
        case OTHER:  return "OTHER";
        default:     return "Unknown";
    }
}
