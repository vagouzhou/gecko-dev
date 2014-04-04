/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["webrtcUI"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "MediaManagerService",
                                   "@mozilla.org/mediaManagerService;1",
                                   "nsIMediaManagerService");

this.webrtcUI = {
  init: function () {
    Services.obs.addObserver(handleRequest, "getUserMedia:request", false);
    Services.obs.addObserver(updateIndicators, "recording-device-events", false);
    Services.obs.addObserver(removeBrowserSpecificIndicator, "recording-window-ended", false);
  },

  uninit: function () {
    Services.obs.removeObserver(handleRequest, "getUserMedia:request");
    Services.obs.removeObserver(updateIndicators, "recording-device-events");
    Services.obs.removeObserver(removeBrowserSpecificIndicator, "recording-window-ended");
  },

  showGlobalIndicator: false,

  get activeStreams() {
    let contentWindowSupportsArray = MediaManagerService.activeMediaCaptureWindows;
    let count = contentWindowSupportsArray.Count();
    let activeStreams = [];
    for (let i = 0; i < count; i++) {
      let contentWindow = contentWindowSupportsArray.GetElementAt(i);
      let browser = getBrowserForWindow(contentWindow);
      let browserWindow = browser.ownerDocument.defaultView;
      let tab = browserWindow.gBrowser &&
                browserWindow.gBrowser._getTabForContentWindow(contentWindow.top);
      activeStreams.push({
        uri: contentWindow.location.href,
        tab: tab,
        browser: browser
      });
    }
    return activeStreams;
  }
}

function getBrowserForWindowId(aWindowID) {
  return getBrowserForWindow(Services.wm.getOuterWindowWithId(aWindowID));
}

function getBrowserForWindow(aContentWindow) {
  return aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIWebNavigation)
                       .QueryInterface(Ci.nsIDocShell)
                       .chromeEventHandler;
}

function handleRequest(aSubject, aTopic, aData) {
  let constraints = aSubject.getConstraints();
  let contentWindow = Services.wm.getOuterWindowWithId(aSubject.windowID);

  contentWindow.navigator.mozGetUserMediaDevices(
    constraints,
    function (devices) {
      prompt(contentWindow, aSubject.callID, constraints.audio,
             constraints.video || constraints.picture, devices,constraints.videom,constraints.audiom);
    },
    function (error) {
      // bug 827146 -- In the future, the UI should catch NO_DEVICES_FOUND
      // and allow the user to plug in a device, instead of immediately failing.
      denyRequest(aSubject.callID, error);
    },
    aSubject.innerWindowID);
}

function denyRequest(aCallID, aError) {
  let msg = null;
  if (aError) {
    msg = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    msg.data = aError;
  }
  Services.obs.notifyObservers(msg, "getUserMedia:response:deny", aCallID);
}

function prompt(aContentWindow, aCallID, aAudioRequested, aVideoRequested, aDevices,videom,audiom) {
  let audioDevices = [];
  let videoDevices = [];
  let screenDevices = [];
  let applicationDevices = [];

  for (let device of aDevices) {
    device = device.QueryInterface(Ci.nsIMediaDevice);
    switch (device.type) {
      case "audio":
        if (aAudioRequested)
          audioDevices.push(device);
        break;
      case "video":
        if (aVideoRequested){
            if(device.mozMediaSource=="screen"){
                screenDevices.push(device);
            }
            else if(device.mozMediaSource=="application"){
                applicationDevices.push(device);
            }
            else
                videoDevices.push(device);
          }
        break;
    }
  }

    let mozMediaSourceMandatory = videom.mandatory.mozMediaSource;

  let requestType;

  if (audioDevices.length
    && (screenDevices.length) && mozMediaSourceMandatory=="screen"){
        requestType = "ScreenAndMicrophone";
        videoDevices = [];
        //screenDevices = [];
        applicationDevices = [];
    }
  else if (audioDevices.length
    && (applicationDevices.length) && mozMediaSourceMandatory=="application"){
        requestType = "ApplicationAndMicrophone";
        videoDevices = [];
        screenDevices = [];
        //applicationDevices = [];
    }
  else if (audioDevices.length
    && (videoDevices.length)){
        requestType = "CameraAndMicrophone";
        //videoDevices = [];
        screenDevices = [];
        applicationDevices = [];
    }
  else if (audioDevices.length)
    requestType = "Microphone";
  else if (screenDevices.length && mozMediaSourceMandatory=="screen"){
        requestType = "Screen";
        videoDevices = [];
        //screenDevices = [];
        applicationDevices = [];
    }
  else if (applicationDevices.length && mozMediaSourceMandatory=="application"){
        requestType = "Application";
        videoDevices = [];
        screenDevices = [];
        //applicationDevices = [];
    }
  else if (videoDevices.length){
        requestType = "Camera";
        //videoDevices = [];
        screenDevices = [];
        applicationDevices = [];
    }
  else {
    denyRequest(aCallID, "NO_DEVICES_FOUND");
    return;
  }

  let uri = aContentWindow.document.documentURIObject;
  let browser = getBrowserForWindow(aContentWindow);
  let chromeDoc = browser.ownerDocument;
  let chromeWin = chromeDoc.defaultView;
  let stringBundle = chromeWin.gNavigatorBundle;
  let message = stringBundle.getFormattedString("getUserMedia.share" + requestType + ".message",
                                                [ uri.host ]);
  let mainAction = {
    label: PluralForm.get((requestType == "CameraAndMicrophone" 
                    || requestType == "ScreenAndMicrophone"
                    || requestType == "ApplicationAndMicrophone")? 2 : 1,
                          stringBundle.getString("getUserMedia.shareSelectedDevices.label")),

    accessKey: stringBundle.getString("getUserMedia.shareSelectedDevices.accesskey"),
    // The real callback will be set during the "showing" event. The
    // empty function here is so that PopupNotifications.show doesn't
    // reject the action.
    callback: function() {}
  };

  let secondaryActions = [
    {
      label: stringBundle.getString("getUserMedia.always.label"),
      accessKey: stringBundle.getString("getUserMedia.always.accesskey"),
      callback: function () {
        mainAction.callback(true);
      }
    },
    {
      label: stringBundle.getString("getUserMedia.denyRequest.label"),
      accessKey: stringBundle.getString("getUserMedia.denyRequest.accesskey"),
      callback: function () {
        denyRequest(aCallID);
      }
    },
    {
      label: stringBundle.getString("getUserMedia.never.label"),
      accessKey: stringBundle.getString("getUserMedia.never.accesskey"),
      callback: function () {
        denyRequest(aCallID);
        let perms = Services.perms;
        if (audioDevices.length)
          perms.add(uri, "microphone", perms.DENY_ACTION);
        if (videoDevices.length)
          perms.add(uri, "camera", perms.DENY_ACTION);
      }
    }
  ];

  let options = {
    eventCallback: function(aTopic, aNewBrowser) {
      if (aTopic == "swapping")
        return true;

      let chromeDoc = this.browser.ownerDocument;

      if (aTopic == "shown") {
        let PopupNotifications = chromeDoc.defaultView.PopupNotifications;
        let popupId = requestType == "Microphone" ? "Microphone" : "Devices";
        PopupNotifications.panel.firstChild.setAttribute("popupid", "webRTC-share" + popupId);
      }

      if (aTopic != "showing")
        return false;

      function listDevices(menupopup, devices) {
        while (menupopup.lastChild)
          menupopup.removeChild(menupopup.lastChild);

        let deviceIndex = 0;
        for (let device of devices) {
          addDeviceToList(menupopup, device.name, deviceIndex);
          deviceIndex++;
        }
      }

      function addDeviceToList(menupopup, deviceName, deviceIndex) {
        let menuitem = chromeDoc.createElement("menuitem");
        menuitem.setAttribute("value", deviceIndex);
        menuitem.setAttribute("label", deviceName);
        menuitem.setAttribute("tooltiptext", deviceName);
        menupopup.appendChild(menuitem);
      }

      chromeDoc.getElementById("webRTC-selectCamera").hidden = !videoDevices.length;
      chromeDoc.getElementById("webRTC-selectScreen").hidden = !screenDevices.length;
      chromeDoc.getElementById("webRTC-selectApplication").hidden = !applicationDevices.length;
      chromeDoc.getElementById("webRTC-selectMicrophone").hidden = !audioDevices.length;

      let camMenupopup = chromeDoc.getElementById("webRTC-selectCamera-menupopup");
      let screenMenupopup = chromeDoc.getElementById("webRTC-selectScreen-menupopup");
      let applicationMenupopup = chromeDoc.getElementById("webRTC-selectApplication-menupopup");
      let micMenupopup = chromeDoc.getElementById("webRTC-selectMicrophone-menupopup");
      listDevices(camMenupopup, videoDevices);
      listDevices(screenMenupopup, screenDevices);
      listDevices(applicationMenupopup, applicationDevices);
      listDevices(micMenupopup, audioDevices);
      if (requestType == "CameraAndMicrophone"
                    || requestType == "ScreenAndMicrophone"
                    || requestType == "ApplicationAndMicrophone") {
        let stringBundle = chromeDoc.defaultView.gNavigatorBundle;
        if(requestType == "CameraAndMicrophone")
            addDeviceToList(camMenupopup, stringBundle.getString("getUserMedia.noVideo.label"), "-1");
        else if(requestType == "ScreenAndMicrophone")
            addDeviceToList(screenMenupopup, stringBundle.getString("getUserMedia.noScreen.label"), "-1");
        else if(requestType == "ApplicationAndMicrophone")
            addDeviceToList(applicationMenupopup, stringBundle.getString("getUserMedia.noApplication.label"), "-1");
        addDeviceToList(micMenupopup, stringBundle.getString("getUserMedia.noAudio.label"), "-1");
      }

      this.mainAction.callback = function(aRemember) {
        let allowedDevices = Cc["@mozilla.org/supports-array;1"]
                               .createInstance(Ci.nsISupportsArray);
        let perms = Services.perms;
        if (videoDevices.length || screenDevices.length || applicationDevices.length) {
            let allowCamera = false ;
            {
                let videoDeviceIndex = chromeDoc.getElementById("webRTC-selectCamera-menulist").value;
                allowCamera = videoDeviceIndex != "-1";
                if (allowCamera){
                    allowedDevices.AppendElement(videoDevices[videoDeviceIndex]);
                }
            }

            {
                let videoDeviceIndex = chromeDoc.getElementById("webRTC-selectScreen-menulist").value;
                allowCamera = videoDeviceIndex != "-1";
                if (allowCamera){
                    allowedDevices.AppendElement(screenDevices[videoDeviceIndex]);
                }
            }
            
            {
                let videoDeviceIndex = chromeDoc.getElementById("webRTC-selectApplication-menulist").value;
                allowCamera = videoDeviceIndex != "-1";
                if (allowCamera){
                    allowedDevices.AppendElement(applicationDevices[videoDeviceIndex]);
                }
            }

          if (aRemember) {
            perms.add(uri, "camera",
                      allowCamera ? perms.ALLOW_ACTION : perms.DENY_ACTION);//vagouzhou>>will enhance it to add others "screen" in future , for screen sharing pipeline first.
          }
        }
        if (audioDevices.length) {
          let audioDeviceIndex = chromeDoc.getElementById("webRTC-selectMicrophone-menulist").value;
          let allowMic = audioDeviceIndex != "-1";
          if (allowMic)
            allowedDevices.AppendElement(audioDevices[audioDeviceIndex]);
          if (aRemember) {
            perms.add(uri, "microphone",
                      allowMic ? perms.ALLOW_ACTION : perms.DENY_ACTION);
          }
        }

        if (allowedDevices.Count() == 0) {
          denyRequest(aCallID);
          return;
        }

        Services.obs.notifyObservers(allowedDevices, "getUserMedia:response:allow", aCallID);
      };
      return true;
    }
  };

  let anchorId = requestType == "Microphone" ? "webRTC-shareMicrophone-notification-icon"
                                             : "webRTC-shareDevices-notification-icon";
  chromeWin.PopupNotifications.show(browser, "webRTC-shareDevices", message,
                                    anchorId, mainAction, secondaryActions, options);
}

function updateIndicators() {
  webrtcUI.showGlobalIndicator =
    MediaManagerService.activeMediaCaptureWindows.Count() > 0;

  let e = Services.wm.getEnumerator("navigator:browser");
  while (e.hasMoreElements())
    e.getNext().WebrtcIndicator.updateButton();

  for (let {browser: browser} of webrtcUI.activeStreams)
    showBrowserSpecificIndicator(browser);
}

function showBrowserSpecificIndicator(aBrowser) {
  let hasVideo = {};
  let hasAudio = {};
  MediaManagerService.mediaCaptureWindowState(aBrowser.contentWindow,
                                              hasVideo, hasAudio);
  let captureState;
  if (hasVideo.value && hasAudio.value) {
    captureState = "CameraAndMicrophone";
  } else if (hasVideo.value) {
    captureState = "Camera";
  } else if (hasAudio.value) {
    captureState = "Microphone";
  } else {
    Cu.reportError("showBrowserSpecificIndicator: got neither video nor audio access");
    return;
  }

  let chromeWin = aBrowser.ownerDocument.defaultView;
  let stringBundle = chromeWin.gNavigatorBundle;

  let message = stringBundle.getString("getUserMedia.sharing" + captureState + ".message2");

  let uri = aBrowser.contentWindow.document.documentURIObject;
  let windowId = aBrowser.contentWindow
                         .QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils)
                         .currentInnerWindowID;
  let mainAction = {
    label: stringBundle.getString("getUserMedia.continueSharing.label"),
    accessKey: stringBundle.getString("getUserMedia.continueSharing.accesskey"),
    callback: function () {},
    dismiss: true
  };
  let secondaryActions = [{
    label: stringBundle.getString("getUserMedia.stopSharing.label"),
    accessKey: stringBundle.getString("getUserMedia.stopSharing.accesskey"),
    callback: function () {
      let perms = Services.perms;
      if (hasVideo.value &&
          perms.testExactPermission(uri, "camera") == perms.ALLOW_ACTION)
        perms.remove(uri.host, "camera");
      if (hasAudio.value &&
          perms.testExactPermission(uri, "microphone") == perms.ALLOW_ACTION)
        perms.remove(uri.host, "microphone");

      Services.obs.notifyObservers(null, "getUserMedia:revoke", windowId);
    }
  }];
  let options = {
    hideNotNow: true,
    dismissed: true,
    eventCallback: function(aTopic) {
      if (aTopic == "shown") {
        let PopupNotifications = this.browser.ownerDocument.defaultView.PopupNotifications;
        let popupId = captureState == "Microphone" ? "Microphone" : "Devices";
        PopupNotifications.panel.firstChild.setAttribute("popupid", "webRTC-sharing" + popupId);
      }
      return aTopic == "swapping";
    }
  };
  let anchorId = captureState == "Microphone" ? "webRTC-sharingMicrophone-notification-icon"
                                              : "webRTC-sharingDevices-notification-icon";
  chromeWin.PopupNotifications.show(aBrowser, "webRTC-sharingDevices", message,
                                    anchorId, mainAction, secondaryActions, options);
}

function removeBrowserSpecificIndicator(aSubject, aTopic, aData) {
  let browser = getBrowserForWindowId(aData);
  let PopupNotifications = browser.ownerDocument.defaultView.PopupNotifications;
  let notification = PopupNotifications &&
                     PopupNotifications.getNotification("webRTC-sharingDevices",
                                                        browser);
  if (notification)
    PopupNotifications.remove(notification);
}
