* **\[MDAPI-472]\[C++]** A subscription warns when its listeners are slow or its events come late, and the
  documentation explains on which threads the listeners are called and why they must not block.
    * When the event listeners of a subscription take longer than 0.1 s for one batch, a warning is reported through
      `ErrorManager` with the subscription (its number and event types), the time and the size of the batch; the
      default handler prints it to `stderr`. The system property `dxfcpp.subscription.slowListenerThreshold`
      (`DXFeedSubscription::SLOW_LISTENER_THRESHOLD_PROPERTY`) changes the threshold, `0` turns the check off.
    * With the system property `dxfcpp.subscription.eventLagThreshold` set (off by default), a warning is reported
      when the newest `Quote`, `Trade`, `TradeETH` or `TimeAndSale` of a received batch is older than the threshold.
      Time series subscriptions are not checked.
    * Each check of a subscription reports at most one warning per minute (`dxfcpp.subscription.warningPeriod`); the
      next warning tells how many excesses were since the previous one and the largest of them. The properties are
      time periods (`0.1s`, `5s`, `1m`) read when a subscription is created.
    * The page "Listeners and threads" of the documentation describes the threads of the listeners and the checks.
