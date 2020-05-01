class Messages {
  HELLO = { id: "0", text: "HELLO" };
  OFFER_TRADE = { id: "1", text: "OFFER_TRADE" };
  ACCEPT_TRADE = { id: "2", text: "ACCEPT_TRADE" };

  offerTrade(give,desire) {
    return {command:this.OFFER_TRADE,give:give,desire:desire}
  }

  acceptTrade(give,desire) {

  }
}