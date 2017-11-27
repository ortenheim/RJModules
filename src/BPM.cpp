#include "RJModules.hpp"

#include "dsp/digital.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

/*
Thank you to MSCHacks for the starting point on this one!
Thanks to Strum for the display widget!
*/

struct BPM: Module {
    enum ParamIds {
        BPM_PARAM,
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_CV_INPUT,
        RESET_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        CH3_OUTPUT,
        CH4_OUTPUT,
        CH5_OUTPUT,
        CH6_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RESET_LIGHT,
        PULSE_LIGHT,
        NUM_LIGHTS
    };

    float resetLight = 0.0;
    float pulseLight = 0.0;
    int m_fBPM = 133.0;
    float m_fBeatsPers;
    float m_fMainClockCount;

    BPM() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
    void    BPMChange( float fbmp, bool bforce );
};

template <typename BASE>
struct BigOlLight : BASE {
        BigOlLight() {
                this->box.size = mm2px(Vec(34, 34));
        }
};

struct NumberDisplayWidget : TransparentWidget {

  int *value;
  std::shared_ptr<Font> font;

  NumberDisplayWidget() {
    font = Font::load(assetPlugin(plugin, "res/Segment7Standard.ttf"));
  };

  void draw(NVGcontext *vg) override
  {
    // Background
    NVGcolor backgroundColor = nvgRGB(0xC0, 0xC0, 0xC0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
    nvgFillColor(vg, backgroundColor);
    nvgFill(vg);

    // text
    nvgFontSize(vg, 32);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 2.5);

    std::stringstream to_display;
    to_display << std::setw(3) << (int) *value;

    Vec textPos = Vec(16.0f, 33.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

void BPM::BPMChange( float fbpm, bool bforce ){

    // // don't change if it is already the same
    if( !bforce && ( (int)(fbpm * 1000.0f ) == (int)(m_fBPM * 1000.0f ) ) )
        return;

    m_fBPM = fbpm;
    m_fBeatsPers = fbpm / 60.0;

    // if( m_pDigitDisplayBPM )
    //    m_pDigitDisplayBPM->SetFloat( m_fBPM );

    // for( int i = 0; i < nCHANNELS; i++ )
    //     CalcChannelClockRate( i );
}

void BPM::step() {

    const float lightLambda = 0.075;
    float output = 0.0;
    SchmittTrigger resetTrigger;

    bool bMainClockTrig = false;
    BPMChange(params[BPM_PARAM].value, false);
    BPMChange(133.0, true);

    // keep track of main bpm
    // m_fMainClockCount += m_fBeatsPers;
    // m_fMainClockCount += (133/60.0);

    //float combined_crush_floor = params[CH2_PARAM].value * clampf(inputs[CH2_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

    // new_value = ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
    float bpm_val = params[BPM_PARAM].value;
    float mapped_bpm = ((bpm_val - 0.0) / (1.0 - 0.0) ) * (400.0 - 70.0) + 70.0;

    m_fBPM = mapped_bpm;
    m_fMainClockCount += (mapped_bpm/60.0);

    if( ( m_fMainClockCount ) >= engineGetSampleRate() )
    {
        m_fMainClockCount = m_fMainClockCount - engineGetSampleRate();
        bMainClockTrig = true;
    }

    if( bMainClockTrig )
    {
        // printf("Trigger!\n");
        output = 12.0;
        resetLight = 1.0;
        pulseLight = 1.0;
    }

    // std::cout << m_fBeatsPers  << "\n";
    // std::cout << m_fMainClockCount  << "\n";

    pulseLight -= pulseLight / lightLambda / engineGetSampleRate();

    outputs[CH1_OUTPUT].value = output;
    outputs[CH2_OUTPUT].value = output;
    outputs[CH3_OUTPUT].value = output;
    outputs[CH4_OUTPUT].value = output;
    outputs[CH5_OUTPUT].value = output;
    outputs[CH6_OUTPUT].value = output;
    lights[PULSE_LIGHT].value = pulseLight;

}

BPMWidget::BPMWidget() {
    BPM *module = new BPM();
    setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/BPM.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    addInput(createInput<PJ301MPort>(Vec(24, 160), module, BPM::CH1_CV_INPUT));
    addInput(createInput<PJ301MPort>(Vec(106, 165), module, BPM::RESET_CV_INPUT));
    addParam(createParam<LEDButton>(Vec(109, 132), module, BPM::RESET_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<GreenLight>>(Vec(113.4, 136.4), module, BPM::RESET_LIGHT));

    addOutput(createOutput<PJ301MPort>(Vec(24, 223), module, BPM::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(65, 223), module, BPM::CH2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(105, 223), module, BPM::CH3_OUTPUT));

    addOutput(createOutput<PJ301MPort>(Vec(24, 274), module, BPM::CH4_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(65, 274), module, BPM::CH5_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(106, 274), module, BPM::CH6_OUTPUT));

    addParam(createParam<RoundBlackKnob>(Vec(58, 140), module, BPM::BPM_PARAM, 0.0, 1.0, 0.0));
    // addChild(createLight<LargeLight<GreenLight>>(Vec(28, 130), module, BPM::PULSE_LIGHT));
    //addChild(createLight<BigOlLight<GreenLight>>(Vec(25, 70), module, BPM::RESET_LIGHT));

    NumberDisplayWidget *display = new NumberDisplayWidget();
    display->box.pos = Vec(28, 70);
    display->box.size = Vec(100, 40);
    display->value = &module->m_fBPM;
    addChild(display);
}