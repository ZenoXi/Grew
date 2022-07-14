#pragma once

#include "ComponentBase.h"
#include "Functions.h"
#include "Transition.h"

#include <fstream>
#include <Windows.h>

#pragma comment(lib, "Winmm.lib")
#include <mmsystem.h>

namespace zcom
{
    // Completely barebones component, only contains base component functionality
    class Waveform : public Base
    {
#pragma region base_class
    protected:
        void _OnUpdate();
        bool _Redraw() { return GetVisible(); }
        void _OnDraw(Graphics g)
        {
            ID2D1SolidColorBrush* brush;
            g.target->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &brush);

            for (int i = 0; i < 498; i++)
            {
                _transitions[i].Apply(_values[i]);
                if (_data.lAmps[i] > _values[i])
                {
                    _values[i] = _data.lAmps[i];
                    _transitions[i].Start(_values[i], 0.0f);
                }

                D2D1_RECT_F rect;
                rect.left = 2 * i + 0.0f;
                rect.right = 2 * i + 2.0f;
                rect.top = 0.0f;
                rect.bottom = _values[i] * 1000.0f;
                g.target->FillRectangle(rect, brush);
            }

            brush->Release();
        }
        void _OnResize(int width, int height) {}

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "waveform"; }
#pragma endregion

    private:
        struct WaveData
        {
            std::unique_ptr<float[]> lAmps;
            std::unique_ptr<float[]> rAmps;
            TimePoint time;

            WaveData()
            {
                lAmps = std::make_unique<float[]>(500);
                rAmps = std::make_unique<float[]>(500);
            }
        };

        WaveData _data;
        std::vector<float> _values;
        std::vector<Transition<float>> _transitions;
        Duration _updateInterval = Duration(25, MILLISECONDS);
        TimePoint _lastUpdate = ztime::Main();

    protected:
        friend class Scene;
        friend class Base;
        Waveform(Scene* scene) : Base(scene) {}
        void Init()
        {
            for (int i = 0; i < 498; i++)
            {
                _values.push_back(0.0f);
                _transitions.push_back(Transition<float>(Duration(200, MILLISECONDS)));
            }
        }
    public:
        ~Waveform() {}
        Waveform(Waveform&&) = delete;
        Waveform& operator=(Waveform&&) = delete;
        Waveform(const Waveform&) = delete;
        Waveform& operator=(const Waveform&) = delete;
    };
}