#ifndef TEXTTOSPEECH_H
#define TEXTTOSPEECH_H

#include <windows.h> // Include Windows API
#include <sapi.h>
#include <sphelper.h>
#include <QString>

class TextToSpeech {
public:
    TextToSpeech() {
        // Initialize COM library and create voice
        CoInitialize(NULL);
        HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&voice);
        if (FAILED(hr)) {
            // Handle error
        }
    }

    ~TextToSpeech() {
        if (voice) {
            voice->Release();
            voice = NULL;
        }
        CoUninitialize();
    }

    bool isSpeechEnabled() const {
        return voice != NULL;
    }

    void speak(const QString &text) {
        if (voice) {
            WCHAR wtext[1024];
            MultiByteToWideChar(CP_UTF8, 0, text.toUtf8().constData(), -1, wtext, 1024);

            // Stop any current speech before speaking the new text
            voice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL);
            voice->Speak(wtext, SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
        }
    }

private:
    ISpVoice *voice = NULL;
};

// Global or member variable
TextToSpeech *tts;


#endif // TEXTTOSPEECH_H
