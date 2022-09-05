#pragma once

#include "App.h"
#include "Button.h"
#include "TextInput.h"
#include "KeyboardEventHandler.h"

#include "GameTime.h"
#include "decimal.h"

#define MAX_DEC_PRECISION 7

typedef dec::decimal<MAX_DEC_PRECISION> NumberInputValue;

namespace zcom
{
    class NumberInput : public TextInput
    {
    public:
        NumberInputValue GetValue() const
        {
            return _value;
        }

        int GetPrecision() const
        {
            return _precision;
        }

        NumberInputValue GetStepSize() const
        {
            return _stepSize;
        }

        NumberInputValue GetMinValue() const
        {
            return _minValue;
        }

        NumberInputValue GetMaxValue() const
        {
            return _maxValue;
        }

        void SetValue(NumberInputValue value)
        {
            if (value == _value)
                return;

            _value = value;
            _BoundValue();
            _UpdateText();
        }

        void SetPrecision(int precision)
        {
            if (precision < 0)
                precision = 0;
            if (precision > MAX_DEC_PRECISION)
                precision = MAX_DEC_PRECISION;
            if (precision == _precision)
                return;

            _precision = precision;
            _UpdateText();
        }

        void SetStepSize(NumberInputValue stepSize)
        {
            _stepSize = stepSize;
        }

        void SetMinValue(NumberInputValue value)
        {
            _minValue = value;
            if (_maxValue < _minValue)
                _maxValue = _minValue;
            _BoundValue();
        }

        void SetMaxValue(NumberInputValue value)
        {
            _maxValue = value;
            if (_minValue > _maxValue)
                _minValue = _maxValue;
            _BoundValue();
        }


    protected:
        friend class Scene;
        friend class Base;
        NumberInput(Scene* scene) : TextInput(scene) {}
        void Init()
        {
            TextInput::Init();
            SetTextAreaMargins({ 0, 0, 19, 0 });

            auto valueUpButton = Create<Button>(L"");
            valueUpButton->SetBaseSize(19, 9);
            valueUpButton->SetVerticalOffsetPixels(-5);
            valueUpButton->SetAlignment(Alignment::END, Alignment::CENTER);
            valueUpButton->SetPreset(ButtonPreset::NO_EFFECTS);
            valueUpButton->SetButtonImageAll(ResourceManager::GetImage("number_inc_color"));
            valueUpButton->SetButtonImage(ResourceManager::GetImage("number_inc"));
            valueUpButton->SetImageStretch(ImageStretchMode::CENTER);
            valueUpButton->SetSelectable(false);
            valueUpButton->SetActivation(zcom::ButtonActivation::PRESS);
            valueUpButton->SetOnActivated([&]()
            {
                _UpdateValue();
                SetValue(_value + _stepSize);
            });

            auto valueDownButton = Create<Button>(L"");
            valueDownButton->SetBaseSize(19, 9);
            valueDownButton->SetVerticalOffsetPixels(5);
            valueDownButton->SetAlignment(Alignment::END, Alignment::CENTER);
            valueDownButton->SetPreset(ButtonPreset::NO_EFFECTS);
            valueDownButton->SetButtonImageAll(ResourceManager::GetImage("number_dec_color"));
            valueDownButton->SetButtonImage(ResourceManager::GetImage("number_dec"));
            valueDownButton->SetImageStretch(ImageStretchMode::CENTER);
            valueDownButton->SetSelectable(false);
            valueDownButton->SetActivation(zcom::ButtonActivation::PRESS);
            valueDownButton->SetOnActivated([&]()
            {
                _UpdateValue();
                SetValue(_value - _stepSize);
            });

            AddItem(valueUpButton.release(), true);
            AddItem(valueDownButton.release(), true);

            _value = 0;
            _minValue = std::numeric_limits<int32_t>::min();
            _maxValue = std::numeric_limits<int32_t>::max();
            _stepSize = 1;
            _UpdateText();

            SetPattern(L"^[-\\.0-9]+$");
            AddOnTextChanged([&](Label* label, std::wstring& newText)
            {
                if (!_internalChange)
                    _UpdateValue();
            }, { this, "number_input" });
        }
    public:
        ~NumberInput()
        {

        }
        NumberInput(NumberInput&&) = delete;
        NumberInput& operator=(NumberInput&&) = delete;
        NumberInput(const NumberInput&) = delete;
        NumberInput& operator=(const NumberInput&) = delete;

    private:
        NumberInputValue _value;
        int _precision = 0;
        NumberInputValue _stepSize;
        NumberInputValue _minValue;
        NumberInputValue _maxValue;

        bool _internalChange = false;

    private:
        void _UpdateValue()
        {
            NumberInputValue number(wstring_to_string(Text()->GetText()));
            SetValue(number);
            _UpdateText();
        }

        void _UpdateText()
        {
            std::ostringstream ss;
            ss << _value;
            std::wstring str = string_to_wstring(ss.str());

            // Cut off unnecessary decimal points
            if (_precision == 0)
                str = str.substr(0, str.length() - (MAX_DEC_PRECISION + 1));
            else
                str = str.substr(0, str.length() - MAX_DEC_PRECISION + _precision);

            _internalChange = true;
            Text()->SetText(str);
            _internalChange = false;
        }

        void _BoundValue()
        {
            if (_value < _minValue)
                SetValue(_minValue);
            else if (_value > _maxValue)
                SetValue(_maxValue);
        }

#pragma region base_class
    protected:
        EventTargets _OnMouseMove(int deltaX, int deltaY)
        {
            return TextInput::_OnMouseMove(deltaX, deltaY);
        }

        void _OnDeselected()
        {
            TextInput::_OnDeselected();
            _UpdateValue();
        }

        EventTargets _OnWheelUp(int x, int y)
        {
            _UpdateValue();
            SetValue(_value + _stepSize);
            return EventTargets().Add(this, x, y);
        }

        EventTargets _OnWheelDown(int x, int y)
        {
            _UpdateValue();
            SetValue(_value - _stepSize);
            return EventTargets().Add(this, x, y);
        }

    public:
        const char* GetName() const { return Name(); }
        static const char* Name() { return "number_input"; }
#pragma endregion
    };
}