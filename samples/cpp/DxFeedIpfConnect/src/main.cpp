// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

/*
 * This sample demonstrates how to subscribe to available symbols using IPF.
 * Use default DXFeed instance for that data feed address is defined by "dxfeed.properties" file.
 */
int main(int argc, char *argv[]) {
    using namespace dxfcpp;
    using namespace std::string_literals;


//  public static void main(String[] args) throws InterruptedException, IOException {
//        if (args.length != 2) {
//            String eventTypeNames = DXFeedConnect.getEventTypeNames(MarketEvent.class);
//            System.err.println("usage: DXFeedIpfConnect <type> <ipf-file>");
//            System.err.println("where: <type>     is dxfeed event type (" + eventTypeNames + ")");
//            System.err.println("       <ipf-file> is name of instrument profiles file");
//            return;
//        }
//        String argType = args[0];
//        String argIpfFile = args[1];
//
//        Class<? extends MarketEvent> eventType = DXFeedConnect.findEventType(argType, MarketEvent.class);
//        DXFeedSubscription<MarketEvent> sub = DXFeed.getInstance().createSubscription(eventType);
//        sub.addEventListener(events -> {
//            for (MarketEvent event : events)
//                System.out.println(event.getEventSymbol() + ": " + event);
//        });
//        sub.addSymbols(getSymbols(argIpfFile));
//        Thread.sleep(Long.MAX_VALUE);
//    }
//
//  private static List<String> getSymbols(String filename) throws IOException {
//        System.out.printf("Reading instruments from %s ...%n", filename);
//        List<InstrumentProfile> profiles = new InstrumentProfileReader().readFromFile(filename);
//        ProfileFilter filter = profile -> {
//            // This is just a sample, any arbitrary filtering may go here.
//            return
//                profile.getType().equals("STOCK") && // stocks
//                profile.getSIC() / 10 == 357 && // Computer And Office Equipment
//                profile.getExchanges().contains("XNYS"); // traded at NYSE
//        };
//        ArrayList<String> result = new ArrayList<>();
//        System.out.println("Selected symbols are:");
//        for (InstrumentProfile profile : profiles)
//            if (filter.accept(profile)) {
//                result.add(profile.getSymbol());
//                System.out.println(profile.getSymbol() + " (" + profile.getDescription() + ")");
//            }
//        return result;
//    }
//
//  private static interface ProfileFilter {
//      public boolean accept(InstrumentProfile profile);
//    }
}
