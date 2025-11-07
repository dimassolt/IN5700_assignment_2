#include <omnetpp.h>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

using namespace omnetpp;

class GarbageVisualizer : public cSimpleModule {
  private:
    struct ValueBinding {
        const char *figureName;
        cTextFigure *figure = nullptr;
        std::function<std::string()> compute;
    };

    std::vector<ValueBinding> delayBindings;

    cLineFigure *cloudHostLine = nullptr;
    cLineFigure *cloudCanLine = nullptr;
    cLineFigure *cloudAnotherCanLine = nullptr;
    cModule *cloudModule = nullptr;
    cModule *hostModule = nullptr;
    cModule *canModule = nullptr;
    cModule *anotherCanModule = nullptr;
    cTextFigure *headingFigure = nullptr;
    std::string scenarioTitle;
    std::string initialText;
    cModule *systemModule = nullptr;
    bool resultsReady = false;

    static cFigure::Point moduleCenter(cModule *module)
    {
        const char *xStr = module->getDisplayString().getTagArg("p", 0);
        const char *yStr = module->getDisplayString().getTagArg("p", 1);
        const double x = xStr && *xStr ? std::atof(xStr) : 0.0;
        const double y = yStr && *yStr ? std::atof(yStr) : 0.0;
        return {x, y};
    }

    static std::string formatResult(double value)
    {
        if (std::isnan(value))
            return "";
        const long long rounded = std::llround(value);
        return std::to_string(rounded);
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

    void updateDelayTexts()
    {
        for (auto &binding : delayBindings) {
            if (!binding.figure || !binding.compute)
                continue;
            const std::string value = binding.compute();
            binding.figure->setText(value.c_str());
        }
    }

  protected:
    virtual void initialize() override {
        initialText = par("initialText").stdstringValue();
        scenarioTitle = par("scenarioTitle").stdstringValue();
        constexpr size_t kMaxInitialText = 256;
        if (initialText.size() > kMaxInitialText) {
            EV_WARN << "visualizer.initialText truncated from " << initialText.size() << " to " << kMaxInitialText << " characters" << endl;
            initialText.resize(kMaxInitialText);
        }
        systemModule = getParentModule();
        cCanvas *canvas = systemModule->getCanvas();
        cloudModule = systemModule->getSubmodule("cloud");
        hostModule = systemModule->getSubmodule("host", 0);
        canModule = systemModule->getSubmodule("can");
        anotherCanModule = systemModule->getSubmodule("anotherCan");

        if (!cloudModule || !hostModule || !canModule || !anotherCanModule)
            throw cRuntimeError("GarbageVisualizer: unable to locate required submodules for line figures");

        auto configureLine = [canvas](const char *name) {
            auto *line = new cLineFigure(name);
            line->setLineColor(cFigure::Color(90, 90, 90));
            line->setLineWidth(2);
            line->setZIndex(-1);
            canvas->addFigure(line);
            return line;
        };

        cloudHostLine = configureLine("cloudHostLine");
        cloudCanLine = configureLine("cloudCanLine");
        cloudAnotherCanLine = configureLine("cloudAnotherCanLine");

        headingFigure = check_and_cast<cTextFigure *>(canvas->getFigure("infoHeading"));
        headingFigure->setText(scenarioTitle.c_str());

        auto registerBinding = [&](const char *figureName, const char *parameterName) {
            ValueBinding binding;
            binding.figureName = figureName;
            binding.figure = check_and_cast<cTextFigure *>(canvas->getFigure(figureName));
            binding.figure->setText(initialText.c_str());
            binding.compute = [this, parameterName]() {
                if (!systemModule || !systemModule->hasPar(parameterName))
                    return std::string();
                return formatResult(systemModule->par(parameterName).doubleValue());
            };
            delayBindings.push_back(std::move(binding));
        };

        registerBinding("smartSlowOutValue", "smartSlowOutResult");
        registerBinding("smartSlowInValue", "smartSlowInResult");
        registerBinding("smartFastOutValue", "smartFastOutResult");
        registerBinding("smartFastInValue", "smartFastInResult");
        registerBinding("canOutValue", "canOutResult");
        registerBinding("canInValue", "canInResult");
        registerBinding("anotherCanOutValue", "anotherCanOutResult");
        registerBinding("anotherCanInValue", "anotherCanInResult");
        registerBinding("cloudSlowOutValue", "cloudSlowOutResult");
        registerBinding("cloudSlowInValue", "cloudSlowInResult");
        registerBinding("cloudFastOutValue", "cloudFastOutResult");
        registerBinding("cloudFastInValue", "cloudFastInResult");

        updateLines();
    }

    virtual void finish() override {
        resultsReady = true;
        updateDelayTexts();
    }

    virtual void handleMessage(cMessage *msg) override {
        delete msg;
    }

    virtual void refreshDisplay() const override
    {
        const_cast<GarbageVisualizer *>(this)->updateLines();
        if (resultsReady)
            const_cast<GarbageVisualizer *>(this)->updateDelayTexts();
    }
};
Define_Module(GarbageVisualizer);
