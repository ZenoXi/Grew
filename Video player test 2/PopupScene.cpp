#include "PopupScene.h"

void PopupScene::_Init(const SceneOptionsBase* options)
{
    PopupSceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PopupSceneOptions*>(options);
    }

    zcom::PROP_Shadow mainPanelShadow;
    mainPanelShadow.blurStandardDeviation = 5.0f;
    mainPanelShadow.color = D2D1::ColorF(0, 0.75f);

    zcom::PROP_Shadow buttonShadow;
    buttonShadow.offsetX = 2.0f;
    buttonShadow.offsetY = 2.0f;
    buttonShadow.blurStandardDeviation = 2.0f;

    if (opt.popupImage)
    {
        _popupImage = Create<zcom::Image>(opt.popupImage);
        _popupImage->SetBaseSize(50, 50);
        _popupImage->SetVerticalAlignment(zcom::Alignment::CENTER);
        _popupImage->SetOffsetPixels(30, -25);
        _popupImage->SetPlacement(zcom::ImagePlacement::CENTER);
        _popupImage->SetPixelSnap(true);
        _popupImage->SetTintColor(opt.popupImageTint);
    }

    _popupText = Create<zcom::Label>(opt.popupText);
    _popupText->SetVerticalAlignment(zcom::Alignment::CENTER);
    if (opt.popupImage)
    {
        _popupText->SetBaseWidth(240);
        _popupText->SetWidth(240);
        _popupText->SetOffsetPixels(90, -25);
    }
    else
    {
        _popupText->SetBaseWidth(300);
        _popupText->SetWidth(300);
        _popupText->SetOffsetPixels(30, -25);
    }
    _popupText->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _popupText->SetHorizontalTextAlignment(zcom::TextAlignment::CENTER);
    _popupText->SetWordWrap(true);
    _popupText->SetFontSize(16.0f);

    // Calculate total content height
    float totalHeight = 50.0f;
    float textHeight = _popupText->GetTextHeight();
    if (textHeight > totalHeight)
        totalHeight = textHeight;
    _popupText->SetBaseHeight(totalHeight);

    _mainPanel = Create<zcom::Panel>();
    _mainPanel->SetBaseSize(360, (int)std::ceilf(totalHeight) + 20 + 80);
    _mainPanel->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::CENTER);
    _mainPanel->SetBackgroundColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
    _mainPanel->SetCornerRounding(5.0f);
    _mainPanel->SetProperty(mainPanelShadow);

    if (!opt.leftButtonText.empty())
    {
        _leftButton = Create<zcom::Button>(opt.leftButtonText);
        _leftButton->SetBaseSize(100, 30);
        _leftButton->SetVerticalOffsetPixels(-20);
        _leftButton->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::END);
        _leftButton->Text()->SetFontSize(18.0f);
        _leftButton->SetBackgroundColor(opt.leftButtonColor);
        _leftButton->SetCornerRounding(5.0f);
        _leftButton->SetProperty(buttonShadow);
        _leftButton->SetOnActivated([&]()
        {
            _closeScene = true;
            _leftClicked = true;
        });
    }

    if (!opt.rightButtonText.empty() && !opt.leftButtonText.empty())
    {
        _rightButton = Create<zcom::Button>(opt.rightButtonText);
        _rightButton->SetBaseSize(100, 30);
        _rightButton->SetVerticalOffsetPixels(-20);
        _rightButton->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::END);
        _rightButton->Text()->SetFontSize(18.0f);
        _rightButton->SetBackgroundColor(opt.rightButtonColor);
        _rightButton->SetCornerRounding(5.0f);
        _rightButton->SetProperty(buttonShadow);
        _rightButton->SetOnActivated([&]()
        {
            _closeScene = true;
            _rightClicked = true;
        });
    }

    if (!_leftButton && !_rightButton)
    {
        _okButton = Create<zcom::Button>(L"Ok");
        _okButton->SetBaseSize(80, 30);
        _okButton->SetVerticalOffsetPixels(-20);
        _okButton->SetAlignment(zcom::Alignment::CENTER, zcom::Alignment::END);
        _okButton->Text()->SetFontSize(18.0f);
        _okButton->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));
        _okButton->SetTabIndex(0);
        _okButton->SetCornerRounding(5.0f);
        _okButton->SetProperty(buttonShadow);
        _okButton->SetOnActivated([&]()
        {
            _closeScene = true;
            _leftClicked = true;
        });
        _okButton->OnSelected();
    }
    else if (_leftButton && _rightButton)
    {
        _leftButton->SetHorizontalOffsetPixels(-60);
        _rightButton->SetHorizontalOffsetPixels(60);
        _leftButton->SetTabIndex(0);
        _rightButton->SetTabIndex(1);
        _leftButton->OnSelected();
    }
    else if (_leftButton)
    {
        _leftButton->SetTabIndex(0);
        _leftButton->OnSelected();
    }

    if (_popupImage)
        _mainPanel->AddItem(_popupImage.get());
    _mainPanel->AddItem(_popupText.get());
    if (_leftButton)
        _mainPanel->AddItem(_leftButton.get());
    if (_rightButton)
        _mainPanel->AddItem(_rightButton.get());
    if (_okButton)
        _mainPanel->AddItem(_okButton.get());

    _canvas->AddComponent(_mainPanel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.2f));

    _closeScene = false;
    _leftClicked = false;
    _rightClicked = false;
}

void PopupScene::_Uninit()
{
    _canvas->ClearComponents();
    _mainPanel->ClearItems();

    _mainPanel = nullptr;
    _popupImage = nullptr;
    _popupText = nullptr;
    _leftButton = nullptr;
    _rightButton = nullptr;
    _okButton = nullptr;
}

void PopupScene::_Focus()
{

}

void PopupScene::_Unfocus()
{

}

void PopupScene::_Update()
{

}

void PopupScene::_Resize(int width, int height)
{

}