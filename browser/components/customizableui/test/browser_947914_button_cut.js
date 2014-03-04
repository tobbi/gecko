/* This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

add_task(function() {
  info("Check cut button existence and functionality");
  var testText = "cut text test";
  let initialLocation = gBrowser.currentURI.spec;

  yield PanelUI.show();

  let cutButton = document.getElementById("cut-button");
  ok(cutButton, "Cut button exists in Panel Menu");
  ok(cutButton.getAttribute("disabled"), "Cut button is disabled");

  // cut text from URL bar
  gURLBar.value = testText;
  gURLBar.focus();
  gURLBar.select();
  yield PanelUI.show();

  ok(!cutButton.hasAttribute("disabled"), "Cut button gets enabled");
  cutButton.click();
  ok(gURLBar.value == "", "Selected text is removed from source when clicking on cut");

  // check that the text was added to the clipboard
  let clipboard = Services.clipboard;
  let transferable = Cc["@mozilla.org/widget/transferable;1"].createInstance(Ci.nsITransferable);
  const globalClipboard = clipboard.kGlobalClipboard;

  transferable.init(null);
  transferable.addDataFlavor("text/unicode");
  clipboard.getData(transferable, globalClipboard);
  let str = {}, strLength = {};
  transferable.getTransferData("text/unicode", str, strLength);
  let clipboardValue = "";

  if (str.value) {
    str.value.QueryInterface(Ci.nsISupportsString);
    clipboardValue = str.value.data;
  }
  is(clipboardValue, testText, "Data was copied to the clipboard.");

  // clear the URL value and the clipboard
  gURLBar.value = "";
  Services.clipboard.emptyClipboard(globalClipboard);

  // restore the tab as it was at the begining of the test
  var tabToRemove = gBrowser.selectedTab;
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(tabToRemove);
});
