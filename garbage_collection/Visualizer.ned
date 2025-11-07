package garbage_collection;

// Draws runtime wiring between the cloud, host and cans while the simulation runs, and publishes the recorded statistics into text figures once results are available.

// @param initialText   Seeds every statistic text figure before results arrive.
// @param scenarioTitle Headline rendered above the statistic panel.

simple GarbageVisualizer
{
    parameters:
        string initialText = default("");
        string scenarioTitle = default("Scenario");
        @display("i=block/app");
}
