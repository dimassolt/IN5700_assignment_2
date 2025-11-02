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

    cLineFigure *cloudHostLine = nullptr;
    cLineFigure *cloudCanLine = nullptr;
    cLineFigure *cloudAnotherCanLine = nullptr;
    cModule *cloudModule = nullptr;
    cModule *hostModule = nullptr;
    cModule *canModule = nullptr;
    cModule *anotherCanModule = nullptr;

    static cFigure::Point moduleCenter(cModule *module)
    {
        const double x = module->getDisplayString().getTagArgAsDouble("p", 0);
        const double y = module->getDisplayString().getTagArgAsDouble("p", 1);
        return {x, y};
    }

    static void updateLine(cLineFigure *line, cModule *a, cModule *b)
    {
        if (!line || !a || !b)
            return;

        line->setStart(moduleCenter(a));
        line->setEnd(moduleCenter(b));
    }

    void updateLines()
    {
        updateLine(cloudHostLine, cloudModule, hostModule);
        updateLine(cloudCanLine, cloudModule, canModule);
        updateLine(cloudAnotherCanLine, cloudModule, anotherCanModule);
    }

  protected:
    virtual void initialize() override {
        std::string initialText = par("initialText").stdstringValue();
        constexpr size_t kMaxInitialText = 256;
        if (initialText.size() > kMaxInitialText) {
            EV_WARN << "visualizer.initialText truncated from " << initialText.size() << " to " << kMaxInitialText << " characters" << endl;
            initialText.resize(kMaxInitialText);
        }
        cCanvas *canvas = getParentModule()->getCanvas();
        cModule *parent = getParentModule();
        cloudModule = parent->getSubmodule("cloud");
        hostModule = parent->getSubmodule("host", 0);
        canModule = parent->getSubmodule("can");
        anotherCanModule = parent->getSubmodule("anotherCan");

        if (!cloudModule || !hostModule || !canModule || !anotherCanModule)
            throw cRuntimeError("GarbageVisualizer: unable to locate required submodules for line figures");

        auto configureLine = [canvas](const char *name) {
            auto *line = new cLineFigure(name);
            line->setLineColor(cFigure::RGBA(90, 90, 90));
            line->setLineWidth(2);
            line->setZIndex(-1);
            canvas->addFigure(line);
            return line;
        };

        cloudHostLine = configureLine("cloudHostLine");
        cloudCanLine = configureLine("cloudCanLine");
        cloudAnotherCanLine = configureLine("cloudAnotherCanLine");

        for (auto &binding : bindings) {
            binding.figure = check_and_cast<cTextFigure *>(canvas->getFigure(binding.figureName));
            binding.figure->setText(initialText.c_str());
        }

        updateLines();
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

    virtual void refreshDisplay() const override
    {
        const_cast<GarbageVisualizer *>(this)->updateLines();
    }
};
Define_Module(GarbageVisualizer);
