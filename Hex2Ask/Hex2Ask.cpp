#include <iostream>
#include <windows.h>
#include <math.h>
#include <MMSystem.h>

#pragma comment (lib, "winmm.lib")

constexpr auto SAMPLING = 192000;
constexpr auto CHANNEL = 2;
constexpr auto BITSPERSAMPLE = 16;
constexpr auto MAXAMP = 32767;
constexpr auto DIV = 4;
constexpr auto BUFFERING = 10;

void createWave(LPWORD lpData, size_t frequency, size_t sampling, WORD amplitude) {
    double wavelength = (double)sampling / (double)frequency;
    //    double pi = 3.14159265359;
    for (int i = 0; i < wavelength; i++) {
        lpData[i] = (WORD)(amplitude * sin((i * 6.28318530718) / wavelength));
    }
}

BOOLEAN isON(unsigned char data, size_t bit, size_t bitLength, size_t value) {
    int oneByte = 8;
    for (int i = 0; i < oneByte; i++) {
        unsigned char mask = 0x80 >> i;
        if (((data & mask) == mask) &&
            //            (bitLength * ((bit - oneByte) + i)) <= (value % (bitLength * bit)) &&
            //            (value % (bitLength * bit)) < (bitLength * ((bit - oneByte + 1) + i))) {
            (bitLength * (bit - oneByte + i)) <= (value % (bitLength * bit)) &&
            (value % (bitLength * bit)) < (bitLength * (bit - oneByte + 1 + i))) {
            return true;
        }
    }
    return false;
}

void addWave(LPWORD param1, LPWORD param2, size_t length) {
    for (int i = 0; i < length; i++) {
        param1[i] += param2[i];
    }
}

DWORD soundOut(
    LPWORD* lpWaveData,
    HWAVEOUT hWaveOut,
    unsigned char data,
    size_t bit,
    size_t cFrequency,
    BOOL reset,
    BOOL control
) {
    LPWORD lpSinWave;
    LPWORD lpWave;
    LPWORD lpWaveTemp;
    size_t i, j;
    size_t cWavelength = SAMPLING / cFrequency;
    size_t dataLength;
    size_t andSequence = 0;
    size_t dWavelength;
    size_t dFrequency;

    dataLength = (size_t)CHANNEL * (BITSPERSAMPLE / 8) * cWavelength * bit / sizeof(WORD);
    if (control == TRUE) {
        andSequence = (size_t)CHANNEL * (BITSPERSAMPLE / 8) * cWavelength / sizeof(WORD);
    }

    lpWave = (LPWORD)calloc(sizeof(WORD), dataLength + andSequence);
    for (i = 0; i < dataLength + andSequence; i++) {
        lpWave[i] = 0;
    }
    lpWaveTemp = (LPWORD)calloc(sizeof(WORD), dataLength + andSequence);
    lpSinWave = (LPWORD)calloc(sizeof(WORD), SAMPLING);

    /*  チャンネル1 ********************************/
    // シリアルデータ
    dFrequency = 21000;
    dWavelength = SAMPLING / dFrequency;
    createWave(lpSinWave, dFrequency, SAMPLING, MAXAMP / 2);
    memset(lpWaveTemp, 0x00, (dataLength + andSequence) * sizeof(WORD));
    for (i = 0, j = 0; i < dataLength; i += CHANNEL, ++j) {
        lpWaveTemp[i] = lpSinWave[j];
        if (j >= dWavelength) { j = 0; }
    }
    for (i = 0; i < dataLength; i += CHANNEL) {
        if (isON(0xff - data, bit, cWavelength * CHANNEL, i)) {
            lpWaveTemp[i] = 0;
        }
    }
    addWave(lpWave, lpWaveTemp, dataLength + andSequence);

    // クロック
    dFrequency = 16000;
    dWavelength = SAMPLING / dFrequency;
    createWave(lpSinWave, dFrequency, SAMPLING, MAXAMP / 2);
    memset(lpWaveTemp, 0x00, (dataLength + andSequence) * sizeof(WORD));
    for (i = 0, j = 0; i < dataLength; i += CHANNEL) {
        lpWaveTemp[i] = lpSinWave[j];
        ++j;
        if (j >= dWavelength) { j = 0; }
    }
    for (i = 0, j = 0; i < dataLength; i += CHANNEL, ++j) {
        if (j < cWavelength / 1.4) { lpWaveTemp[i] = 0; }
        if (j >= cWavelength) { j = 0; }
    }
    addWave(lpWave, lpWaveTemp, dataLength + andSequence);

    /*  チャンネル2 ********************************/
    // 制御データ
    if (andSequence > 0) {
        dFrequency = 21000;
        dWavelength = SAMPLING / dFrequency;
        createWave(lpSinWave, dFrequency, SAMPLING, MAXAMP / 1.0);
        memset(lpWaveTemp, 0x00, (dataLength + andSequence) * sizeof(WORD));
        for (i = 1 + dataLength + andSequence / 2, j = 0; i < dataLength + andSequence; i += CHANNEL, ++j) {
            lpWaveTemp[i] = lpSinWave[j];
            if (j >= dWavelength) { j = 0; }
        }
        addWave(lpWave, lpWaveTemp, dataLength + andSequence);
    }

    // リセット
    if (reset == TRUE) {
        dFrequency = 16000;
        dWavelength = SAMPLING / dFrequency;
        createWave(lpSinWave, dFrequency, SAMPLING, MAXAMP / 1.0);
        memset(lpWaveTemp, 0x00, (dataLength + andSequence) * sizeof(WORD));
        for (i = 1, j = 0; i < cWavelength / 2; i += CHANNEL, ++j) {
            lpWaveTemp[i] = lpSinWave[j];
            if (j >= dWavelength) { j = 0; }
        }
        addWave(lpWave, lpWaveTemp, dataLength + andSequence);
    }

    free(lpSinWave);
    free(lpWaveTemp);

    *lpWaveData = lpWave;
    return (DWORD)(dataLength + andSequence) * sizeof(WORD);
}

int main() {

    LPWORD lpWave[BUFFERING];
    WAVEHDR whdr[BUFFERING];
    WAVEFORMATEX wfe;
    HWAVEOUT hWaveOut;

    wfe.wFormatTag = WAVE_FORMAT_PCM;
    wfe.nChannels = CHANNEL;
    wfe.wBitsPerSample = BITSPERSAMPLE;
    wfe.nBlockAlign = CHANNEL * BITSPERSAMPLE / 8;
    wfe.nSamplesPerSec = SAMPLING;
    wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;

    for (int i = 0; i < BUFFERING; i++) {
        whdr[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        whdr[i].dwLoops = 1;
        lpWave[i] = NULL;
    }

    std::string str;
    std::cout << "2桁毎の16進コードを入力しEnterを押して下さい。（00 ～ FF）" << std::endl;
    std::cout << "続けて入力する場合は間を空けないでください。" << std::endl;
    std::cout << "Ctrl+Zを入力しEnterで終了します。" << std::endl;
    std::cin >> str;

    waveOutOpen(&hWaveOut, 0, &wfe, 0, 0, CALLBACK_NULL);

    char strData[3];
    strData[2] = 0x00;
    size_t bit = 9;
    // 変調波
    size_t frequency = 10;
    int freeBuffer = 0;

    for (int i = 0, j = 0, k = 0; i < str.size(); i += 2, j++, k++) {
        if (j >= BUFFERING) {
            j = 0;
        }

        freeBuffer = j + 1;
        if (freeBuffer >= BUFFERING) {
            freeBuffer = 0;
        }
        if (lpWave[freeBuffer] != NULL) {
            free(lpWave[freeBuffer]);
            lpWave[freeBuffer] = NULL;
        }

        strData[0] = str[i];
        strData[1] = str[(size_t)i + 1];
        // 入力された文字列をデータに変換
        unsigned char data = (unsigned char)strtol(strData, NULL, 16);

        // サウンド出力
        if (j % 3 == 0) {
            whdr[j].dwBufferLength = soundOut(&lpWave[j], hWaveOut, data, bit, frequency, TRUE, FALSE);
        }
        else if (j % 3 == 2) {
            whdr[j].dwBufferLength = soundOut(&lpWave[j], hWaveOut, data, bit - 1, frequency, FALSE, TRUE);
        }
        else {
            whdr[j].dwBufferLength = soundOut(&lpWave[j], hWaveOut, data, bit - 1, frequency, FALSE, FALSE);
        }

        whdr[j].lpData = (LPSTR)lpWave[j];
        waveOutPrepareHeader(hWaveOut, &whdr[j], sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &whdr[j], sizeof(WAVEHDR));

        if (k > BUFFERING - 3) {
            Sleep((1000 * (DWORD)bit) / (DWORD)frequency);
        }
    }

    int num = (int)str.size() / 2;
    if (num == 0) {
        num = 1;
    }
    else if (num > BUFFERING) {
        num = BUFFERING;
    }
    Sleep((1000 * (DWORD)bit * (num + 1)) / (DWORD)frequency);

    waveOutReset(hWaveOut);
    waveOutClose(hWaveOut);

    for (int i = 0; i < num; i++) {
        if (lpWave[i] != NULL) {
            free(lpWave[i]);
            lpWave[i] = NULL;
        }
    }
}