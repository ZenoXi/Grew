#pragma once
#pragma comment( lib,"dwrite.lib" )

#include "Dwrite.h"

#include "ComponentBase.h"
#include "GameTime.h"

#include <sstream>

namespace zcom
{
    class PlayButton : public Base
    {
#pragma region base_class
    private:
        void _OnUpdate()
        {

        }

        void _OnDraw(Graphics g)
        {
            // Create resources
            if (!_primaryColorBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &_primaryColorBrush);
                g.refs->push_back((IUnknown**)&_primaryColorBrush);
            }
            if (!_blankBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(0, 0.0f), &_blankBrush);
                g.refs->push_back((IUnknown**)&_blankBrush);
            }
            if (!_opacityBrush)
            {
                g.target->CreateSolidColorBrush(D2D1::ColorF(0, 1.0f), &_opacityBrush);
                g.refs->push_back((IUnknown**)&_opacityBrush);
            }
            if (!_playButton) {
                // Create triangle shape
                g.factory->CreatePathGeometry(&_playButton);
                g.refs->push_back((IUnknown**)&_playButton);

                ID2D1GeometrySink* pSink = NULL;
                _playButton->Open(&pSink);

                float rectWidth = 8.0f;
                float rectHeight = 10.0f;
                D2D1_POINT_2F pos1 = { GetWidth() * 0.5f - rectWidth, GetHeight() * 0.5f - rectHeight };
                D2D1_POINT_2F pos2 = { GetWidth() * 0.5f + rectWidth, GetHeight() * 0.5f };
                D2D1_POINT_2F pos3 = { GetWidth() * 0.5f - rectWidth, GetHeight() * 0.5f + rectHeight };

                pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
                pSink->BeginFigure(
                    pos1,
                    D2D1_FIGURE_BEGIN_FILLED
                );
                D2D1_POINT_2F points[3] = {
                    pos1,
                    pos2,
                    pos3
                };
                pSink->AddLines(points, ARRAYSIZE(points));
                pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
                pSink->Close();
                pSink->Release();
            }

            // Draw
            if (_paused)
            {
                if (GetMouseInside() && false)
                {
                    //g->Clear();
                    g.target->Clear(_primaryColorBrush->GetColor());
                    g.target->FillGeometry(_playButton, _blankBrush);
                }
                else
                {
                    g.target->Clear();
                    g.target->DrawRectangle(D2D1::RectF(0.5f, 0.5f, GetWidth() - 0.5f, GetHeight() - 0.5f), _primaryColorBrush);
                    g.target->FillGeometry(_playButton, _primaryColorBrush);

                    //float rectWidth = 8.0f;
                    //float rectHeight = 10.0f;
                    //D2D1_POINT_2F pos1 = { GetWidth() * 0.5f - rectWidth, GetHeight() * 0.5f - rectHeight };
                    //D2D1_POINT_2F pos2 = { GetWidth() * 0.5f + rectWidth, GetHeight() * 0.5f };
                    //D2D1_POINT_2F pos3 = { GetWidth() * 0.5f - rectWidth, GetHeight() * 0.5f + rectHeight };
                    //g.target->DrawLine(pos1, pos2, _primaryColorBrush, 1.5f);
                    //g.target->DrawLine(pos2, pos3, _primaryColorBrush, 1.5f);
                    //g.target->DrawLine(pos3, pos1, _primaryColorBrush, 1.5f);
                }
            }
            else
            {
                int rectSize = 10;
                if (GetMouseInside() && false)
                {
                    g.target->Clear(_primaryColorBrush->GetColor());
                    g.target->FillRectangle(
                        D2D1::RectF(
                            int(GetWidth() * 0.5f - rectSize),
                            int(GetHeight() * 0.5f - rectSize),
                            int(GetWidth() * 0.5 - rectSize + rectSize * 2 / 3),
                            int(GetHeight() * 0.5f + rectSize)
                        ),
                        _blankBrush
                    );
                    g.target->FillRectangle(
                        D2D1::RectF(
                            int(GetWidth() * 0.5f + rectSize - rectSize * 2 / 3),
                            int(GetHeight() * 0.5f - rectSize),
                            int(GetWidth() * 0.5 + rectSize),
                            int(GetHeight() * 0.5f + rectSize)
                        ),
                        _blankBrush
                    );
                }
                else
                {
                    g.target->Clear();
                    g.target->DrawRectangle(D2D1::RectF(0.5f, 0.5f, GetWidth() - 0.5f, GetHeight() - 0.5f), _primaryColorBrush);
                    g.target->FillRectangle(
                        D2D1::RectF(
                            int(GetWidth() * 0.5f - rectSize),
                            int(GetHeight() * 0.5f - rectSize),
                            int(GetWidth() * 0.5 - rectSize + rectSize * 2 / 3),
                            int(GetHeight() * 0.5f + rectSize)
                        ),
                        _primaryColorBrush
                    );
                    g.target->FillRectangle(
                        D2D1::RectF(
                            int(GetWidth() * 0.5f + rectSize - rectSize * 2 / 3),
                            int(GetHeight() * 0.5f - rectSize),
                            int(GetWidth() * 0.5 + rectSize),
                            int(GetHeight() * 0.5f + rectSize)
                        ),
                        _primaryColorBrush
                    );
                }
            }
        }

        void _OnResize(int width, int height)
        {

        }

        Base* _OnMouseMove(int x, int y)
        {
            return this;
        }

        void _OnMouseLeave()
        {

        }

        void _OnMouseEnter()
        {

        }

        Base* _OnLeftPressed(int x, int y)
        {
            _clicked = true;
            if (_paused)
            {
                _paused = false;
            }
            else
            {
                _paused = true;
            }
            return this;
        }

        Base* _OnLeftReleased(int x, int y)
        {
            return this;
        }

        Base* _OnRightPressed(int x, int y)
        {
            return this;
        }

        Base* _OnRightReleased(int x, int y)
        {
            return this;
        }

        Base* _OnWheelUp(int x, int y)
        {
            return nullptr;
        }

        Base* _OnWheelDown(int x, int y)
        {
            return nullptr;
        }

    public:
        std::list<Base*> GetChildren()
        {
            return std::list<Base*>();
        }

        std::list<Base*> GetAllChildren()
        {
            return std::list<Base*>();
        }

        Base* IterateTab()
        {
            if (!Selected())
                return this;
            else
                return nullptr;
        }

        const char* GetName() const { return "play_button"; }
#pragma endregion

    private:
        bool _paused = false;
        bool _clicked = false;

        ID2D1SolidColorBrush* _primaryColorBrush = nullptr;
        ID2D1SolidColorBrush* _blankBrush = nullptr;
        ID2D1SolidColorBrush* _opacityBrush = nullptr;
        ID2D1PathGeometry* _playButton = nullptr;

    public:
        PlayButton()
        {

        }
        ~PlayButton()
        {
            SafeFullRelease((IUnknown**)&_primaryColorBrush);
            SafeFullRelease((IUnknown**)&_blankBrush);
            SafeFullRelease((IUnknown**)&_opacityBrush);
            SafeFullRelease((IUnknown**)&_playButton);
        }
        PlayButton(PlayButton&&) = delete;
        PlayButton& operator=(PlayButton&&) = delete;
        PlayButton(const PlayButton&) = delete;
        PlayButton& operator=(const PlayButton&) = delete;

        bool GetPaused() const
        {
            return _paused;
        }

        void SetPaused(bool paused)
        {
            _paused = paused;
        }

        bool Clicked()
        {
            bool value = _clicked;
            _clicked = false;
            return value;
        }
    };
}