#include "dsp/digital.hpp"
#include <iostream>
#include "RJModules.hpp"

struct LowFrequencyOscillator {
    float phase = 0.0;
    float pw = 0.5;
    float freq = 1.0;
    bool offset = false;
    bool invert = false;

    SchmittTrigger resetTrigger;
    LowFrequencyOscillator() {
        resetTrigger.setThresholds(0.0, 0.01);
    }

    void setPitch(float pitch) {
        pitch = fminf(pitch, 8.0);
        freq = powf(2.0, pitch);
    }

    float getFreq() {
        return freq;
    }

    // void setPulseWidth(float pw_) {
    //     const float pwMin = 0.01;
    //     pw = clampf(pw_, pwMin, 1.0 - pwMin);
    // }
    void setReset(float reset) {
        if (resetTrigger.process(reset)) {
            phase = 0.0;
        }
    }
    void step(float dt) {
        float deltaPhase = fminf(freq * dt, 0.5);
        phase += deltaPhase;
        if (phase >= 1.0)
            phase -= 1.0;
    }
    // float sin() {
    //     if (offset)
    //         return 1.0 - cosf(2*M_PI * phase) * (invert ? -1.0 : 1.0);
    //     else
    //         return sinf(2*M_PI * phase) * (invert ? -1.0 : 1.0);
    // }
    // float tri(float x) {
    //     return 4.0 * fabsf(x - roundf(x));
    // }
    // float tri() {
    //     if (offset)
    //         return tri(invert ? phase - 0.5 : phase);
    //     else
    //         return -1.0 + tri(invert ? phase - 0.25 : phase - 0.75);
    // }
    float saw(float x) {
        return 2.0 * (x - roundf(x));
    }
    float saw() {
        if (offset)
            return invert ? 2.0 * (1.0 - phase) : 2.0 * phase;
        else
            return saw(phase) * (invert ? -1.0 : 1.0);
    }
    // float sqr() {
    //     float sqr = (phase < pw) ^ invert ? 1.0 : -1.0;
    //     return offset ? sqr + 1.0 : sqr;
    // }
    float light() {
        return sinf(2*M_PI * phase);
    }
};


struct Supersaw : Module {
    enum ParamIds {
        OFFSET_PARAM,
        INVERT_PARAM,
        FREQ_PARAM,
        DETUNE_PARAM,

        NUM_PARAMS
    };
    enum InputIds {
        FM1_INPUT,
        FM2_INPUT,
        RESET_INPUT,
        PW_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SAW_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PHASE_POS_LIGHT,
        PHASE_NEG_LIGHT,
        NUM_LIGHTS
    };

    LowFrequencyOscillator oscillator;
    LowFrequencyOscillator oscillator2;
    LowFrequencyOscillator oscillator3;

    Supersaw() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};


void Supersaw::step() {

    float root_pitch = params[FREQ_PARAM].value;
    oscillator.setPitch(root_pitch);
    oscillator.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator.step(1.0 / engineGetSampleRate());
    oscillator.setReset(inputs[RESET_INPUT].value);

    oscillator2.setPitch(root_pitch + (params[DETUNE_PARAM].value * .075));
    oscillator2.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator2.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator2.step(1.0 / engineGetSampleRate());
    oscillator2.setReset(inputs[RESET_INPUT].value);

    oscillator3.setPitch(root_pitch - (params[DETUNE_PARAM].value * .075));
    oscillator3.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator3.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator3.step(1.0 / engineGetSampleRate());
    oscillator3.setReset(inputs[RESET_INPUT].value);

    outputs[SAW_OUTPUT].value = 5.0 * (( oscillator.saw() + oscillator2.saw() + oscillator3.saw() ) / 3);

    lights[PHASE_POS_LIGHT].setBrightnessSmooth(fmaxf(0.0, oscillator2.light()));
    lights[PHASE_NEG_LIGHT].setBrightnessSmooth(fmaxf(0.0, -oscillator2.light()));
}

SupersawWidget::SupersawWidget() {
    Supersaw *module = new Supersaw();
    setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Supersaw.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(createParam<CKSS>(Vec(15, 77), module, Supersaw::OFFSET_PARAM, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(119, 77), module, Supersaw::INVERT_PARAM, 0.0, 1.0, 1.0));

    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, Supersaw::FREQ_PARAM, 0.0, 8.0, 5.0));
    addParam(createParam<RoundBlackKnob>(Vec(23, 143), module, Supersaw::DETUNE_PARAM, 0.0, 1.0, 0.0));

    addInput(createInput<PJ301MPort>(Vec(80, 276), module, Supersaw::RESET_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(80, 320), module, Supersaw::SAW_OUTPUT));

    addChild(createLight<SmallLight<GreenRedLight>>(Vec(99, 42.5), module, Supersaw::PHASE_POS_LIGHT));
}