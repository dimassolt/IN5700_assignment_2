#include <omnetpp.h>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

using namespace omnetpp;

/**
 * Renders live wiring between system modules and displays scenario metrics
 * after the simulation completes.
 */
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

    /** Converts a recorded statistic to a rounded printable string. */
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

    /** Fetches the top-level system module, throwing if the cast fails. */
    cModule *requireSystemModule()
    {
        if (auto *module = getParentModule())
            return module;
        throw cRuntimeError("GarbageVisualizer: missing parent system module");
    }

    /** Returns the submodule with the given name, throwing on failure. */
    cModule *requireSubmodule(const char *name, int index = -1)
    {
        cModule *owner = systemModule;
        cModule *found = (index >= 0) ? owner->getSubmodule(name, index) : owner->getSubmodule(name);
        if (!found)
            throw cRuntimeError("GarbageVisualizer: unable to locate submodule '%s'", name);
        return found;
    }

    /** Locates a mandatory text figure on the canvas. */
    cTextFigure *requireTextFigure(const char *figureName)
    {
        cCanvas *canvas = systemModule->getCanvas();
        auto *figure = canvas->getFigure(figureName);
        if (!figure)
            throw cRuntimeError("GarbageVisualizer: missing figure '%s'", figureName);
        return check_and_cast<cTextFigure *>(figure);
    }

    /** Helper for configuring the lines linking cloud to other modules. */
    cLineFigure *createLinkLine(const char *name)
    {
        cCanvas *canvas = systemModule->getCanvas();
        auto *line = new cLineFigure(name);
        line->setLineColor(cFigure::Color(90, 90, 90));
        line->setLineWidth(2);
        line->setZIndex(-1);
        canvas->addFigure(line);
        return line;
    }

    void registerBinding(const char *figureName, const char *parameterName)
    {
        ValueBinding binding;
        binding.figureName = figureName;
        binding.figure = requireTextFigure(figureName);
        binding.figure->setText(initialText.c_str());
        if (!systemModule->hasPar(parameterName)) {
            EV_WARN << "GarbageVisualizer: statistic parameter '" << parameterName
                    << "' not found; figure '" << figureName << "' will stay blank" << endl;
            binding.compute = []() { return std::string(); };
        }
        else {
            binding.compute = [this, parameterName]() {
                return formatResult(systemModule->par(parameterName).doubleValue());
            };
        }
        delayBindings.push_back(std::move(binding));
    }

  protected:
    void initialize() override
    {
        initialText = par("initialText").stdstringValue();
        scenarioTitle = par("scenarioTitle").stdstringValue();
        constexpr size_t kMaxInitialText = 256;
        if (initialText.size() > kMaxInitialText) {
            EV_WARN << "visualizer.initialText truncated from " << initialText.size() << " to " << kMaxInitialText << " characters" << endl;
            initialText.resize(kMaxInitialText);
        }
        systemModule = requireSystemModule();

        cloudModule = requireSubmodule("cloud");
        hostModule = requireSubmodule("host", 0);
        canModule = requireSubmodule("can");
        anotherCanModule = requireSubmodule("anotherCan");

        cloudHostLine = createLinkLine("cloudHostLine");
        cloudCanLine = createLinkLine("cloudCanLine");
        cloudAnotherCanLine = createLinkLine("cloudAnotherCanLine");

        headingFigure = requireTextFigure("infoHeading");
        headingFigure->setText(scenarioTitle.c_str());

        static const struct {
            const char *figureName;
            const char *parameterName;
        } kBindingSpecs[] = {
            {"smartSlowOutValue", "smartSlowOutResult"},
            {"smartSlowInValue", "smartSlowInResult"},
            {"smartFastOutValue", "smartFastOutResult"},
            {"smartFastInValue", "smartFastInResult"},
            {"canOutValue", "canOutResult"},
            {"canInValue", "canInResult"},
            {"anotherCanOutValue", "anotherCanOutResult"},
            {"anotherCanInValue", "anotherCanInResult"},
            {"cloudSlowOutValue", "cloudSlowOutResult"},
            {"cloudSlowInValue", "cloudSlowInResult"},
            {"cloudFastOutValue", "cloudFastOutResult"},
            {"cloudFastInValue", "cloudFastInResult"}
        };

        const size_t bindingCount = sizeof(kBindingSpecs) / sizeof(kBindingSpecs[0]);
        delayBindings.reserve(delayBindings.size() + bindingCount);
        for (size_t i = 0; i < bindingCount; ++i)
            registerBinding(kBindingSpecs[i].figureName, kBindingSpecs[i].parameterName);

        updateLines();
    }

    void finish() override
    {
        resultsReady = true;
        updateDelayTexts();
    }

    void handleMessage(cMessage *msg) override
    {
        delete msg;
    }

    void refreshDisplay() const override
    {
        const_cast<GarbageVisualizer *>(this)->updateLines();
        if (resultsReady)
            const_cast<GarbageVisualizer *>(this)->updateDelayTexts();
    }
};
Define_Module(GarbageVisualizer);
