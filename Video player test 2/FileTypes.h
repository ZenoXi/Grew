#pragma once

#include "Functions.h"

#include <vector>
#include <string>

// The file types which are likely to be supported by ffmpeg
// These file types are taken from VLC github

#define EXTENSIONS_VIDEO "\
*.3g2;*.3gp;*.3gp2;*.3gpp;*.amrec;\
*.amv;*.asf;*.avi;*.bik;*.bin;\
*.crf;*.dav;*.divx;*.drc;*.dv;\
*.dvr-ms;*.evo;*.f4v;*.flv;\
*.gvi;*.gxf;*.iso;*.m1v;*.m2v;\
*.m2t;*.m2ts;*.m4v;*.mkv;*.mov;\
*.mp2;*.mp2v;*.mp4;*.mp4v;*.mpe;\
*.mpeg;*.mpeg1;*.mpeg2;*.mpeg4;*.mpg;\
*.mpv2;*.mts;*.mtv;*.mxf;*.mxg;\
*.nsv;*.nuv;*.ogg;*.ogm;*.ogv;\
*.ogx;*.ps;*.rec;*.rm;*.rmvb;\
*.rpl;*.thp;*.tod;*.tp;*.ts;\
*.tts;*.txd;*.vob;*.vro;*.webm;\
*.wm;*.wmv;*.wtv;*.xesc"


#define EXTENSIONS_AUDIO "\
*.3ga;*.669;*.a52;*.aac;*.ac3;\
*.adt;*.adts;*.aif;*.aifc;*.aiff;\
*.amb;*.amr;*.aob;*.ape;*.au;\
*.awb;*.caf;*.dts;*.dsf;*.dff;\
*.flac;*.it;*.kar;*.m4a;*.m4b;\
*.m4p;*.m5p;*.mid;*.mka;*.mlp;\
*.mod;*.mpa;*.mp1;*.mp2;*.mp3;\
*.mpc;*.mpga;*.mus;*.oga;*.ogg;\
*.oma;*.opus;*.qcp;*.ra;*.rmi;\
*.s3m;*.sid;*.spx;*.tak;*.thd;\
*.tta;*.voc;*.vqf;*.w64;*.wav;\
*.wma;*.wv;*.xa;*.xm"

#define EXTENSIONS_SUBTITLES "\
*.srt;*.ssa,*.ass,*.vtt,*.ttml"

static const std::vector<std::pair<std::wstring, std::wstring>> SUPORTED_MEDIA_FILE_TYPES_WINAPI
{
    { L"Media files", L"" EXTENSIONS_VIDEO ";" EXTENSIONS_AUDIO },
    { L"Video files", L"" EXTENSIONS_VIDEO },
    { L"Audio files", L"" EXTENSIONS_AUDIO },
    { L"All files", L"*.*" }
};

static std::vector<std::wstring> ExtractPureExtensions(std::wstring extstr)
{
    std::vector<std::wstring> pureExts;
    // Split along ';'
    split_wstr(extstr, pureExts, L';', true);

    // Remove the '*.' prefix
    for (int i = 0; i < pureExts.size(); i++)
    {
        if (pureExts[i].length() < 3)
        {
            pureExts.erase(pureExts.begin() + i);
            i--;
            continue;
        }

        if (pureExts[i][0] == L'*' && pureExts[i][1] == L'.')
            pureExts[i].erase(0, 2);
    }

    return pureExts;
}