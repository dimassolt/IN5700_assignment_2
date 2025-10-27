#include <omnetpp.h>
#include <array>
#include <string>

using namespace omnetpp;

class GarbageVisualizer : public cSimpleModule {
  private:
    struct ValueBinding {
        const char *figureName;
        const char *finalText;
        cTextFigure *figure = nullptr;
    };

    std::array<ValueBinding, 12> bindings{{
        ValueBinding{"smartSlowOutValue", "0"},
        ValueBinding{"smartSlowInValue", "0"},
        ValueBinding{"smartFastOutValue", "800"},
        ValueBinding{"smartFastInValue", "200"},
        ValueBinding{"canOutValue", "100"},
        ValueBinding{"canInValue", "100"},
        ValueBinding{"anotherCanOutValue", "100"},
        ValueBinding{"anotherCanInValue", "100"},
        ValueBinding{"cloudSlowOutValue", "0"},
        ValueBinding{"cloudSlowInValue", "0"},
        ValueBinding{"cloudFastOutValue", "0"},
        ValueBinding{"cloudFastInValue", "0"}
    }};

  protected:
    virtual void initialize() override {
        std::string initialText = par("initialText").stdstringValue();
        constexpr size_t kMaxInitialText = 256;
        if (initialText.size() > kMaxInitialText) {
            EV_WARN << "visualizer.initialText truncated from " << initialText.size() << " to " << kMaxInitialText << " characters" << endl;
            initialText.resize(kMaxInitialText);
        }
        cCanvas *canvas = getParentModule()->getCanvas();
        for (auto &binding : bindings) {
            binding.figure = check_and_cast<cTextFigure *>(canvas->getFigure(binding.figureName));
            binding.figure->setText(initialText.c_str());
        }
    }

    virtual void finish() override {
        for (auto &binding : bindings) {
            if (binding.figure)
                binding.figure->setText(binding.finalText);
        }
    }

    virtual void handleMessage(cMessage *msg) override {
        delete msg;
    }
};
Define_Module(GarbageVisualizer);
