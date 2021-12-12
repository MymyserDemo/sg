#include <iostream>
#include <windows.h>
#include <math.h>
#include <MMSystem.h>

#pragma comment (lib, "winmm.lib")

void createWave(LPWORD lpData, size_t frequency, size_t sampling, WORD amplitude) {
    size_t wavelength = sampling / frequency;
    double d = 360.0 / wavelength;
    double pi = 3.14159265359;
    for (int i = 0; i < wavelength; i++) {
        lpData[i] = (WORD)(amplitude * sin(d * (i % wavelength) / 180.0 * pi));
    }
}

int main() {

    WAVEFORMATEX wfe;
    static HWAVEOUT hWaveOut;
    static WAVEHDR whdr;
    static LPWORD lpWave;
    static LPWORD lpData;

    // 最初と最後の1sは出力しないので3以上とする
    size_t terms = 7;

    size_t i, j, start, end;
    size_t frequency = 400;
    size_t sampling = 192000;
    size_t wavelength = sampling / frequency;

    wavelength = sampling / frequency;
    wfe.wFormatTag = WAVE_FORMAT_PCM;
    wfe.nChannels = 2;
    wfe.wBitsPerSample = 16;
    wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;
    wfe.nSamplesPerSec = (DWORD)sampling;
    wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
    waveOutOpen(&hWaveOut, 0, &wfe, 0, 0, CALLBACK_NULL);
    lpWave = (LPWORD)calloc(sizeof(WORD), wfe.nChannels * sampling * terms);

    end = sampling * wfe.nChannels * 1;
    WORD amplitude = 32767;
    for (i = 0; i < end; i++) {
        lpWave[i] = 0;
    }

    // 最初の1sは出力しない
    start = sampling * wfe.nChannels * 1;
    end = wfe.nChannels * sampling * terms;
    // 最後の1sは出力しない
    end -= sampling * wfe.nChannels * 1;

    size_t valWavelength;
    size_t valFrequency;

    lpData = (LPWORD)calloc(sizeof(WORD), wavelength);
    createWave(lpData, frequency, sampling, amplitude);
    for (i = start, j = 0; i < end; i += 2) {
        lpWave[i] = lpData[j];
        ++j;
        if (j >= wavelength) { j = 0; }
    }
    free(lpData);

    for (i = start + 1, j = 0; i < end; i += 2) {
        if (j == 1) {
            valFrequency = frequency + ((i - start) * (21000.0 - frequency)) / (sampling * wfe.nChannels * (terms - 2));
            valWavelength = sampling / valFrequency;
            lpData = (LPWORD)calloc(sizeof(WORD), valWavelength);
            createWave(lpData, valFrequency, sampling, amplitude);
        }
        lpWave[i] = lpData[j];
        ++j;
        if (j >= valWavelength) {
            j = 0;
            free(lpData);
        }
    }

    whdr.lpData = (LPSTR)lpWave;
    whdr.dwBufferLength = wfe.nAvgBytesPerSec * terms;
    whdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    whdr.dwLoops = 1;
    waveOutPrepareHeader(hWaveOut, &whdr, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &whdr, sizeof(WAVEHDR));

    char str[128];
    std::cout << "hello, world\n";
    std::cin >> str;

}