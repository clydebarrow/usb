;wait for new hardware wizard
WinWait("Found New Hardware Wizard", "", 60)
ControlCommand("Found New Hardware Wizard", "", 8105, "Check")
ControlClick("Found New Hardware Wizard", "", 12324);
WinWait("Found New Hardware Wizard", "software automatically", 60)
ControlCommand("Found New Hardware Wizard", "", 1049, "Check")
ControlClick("Found New Hardware Wizard", "", 12324)
WinWait("Hardware Installation", "", 60);
ControlClick("Hardware Installation", "", 5303);
WinWait("Found New Hardware Wizard", "finished installing", 60)
ControlClick("Found New Hardware Wizard", "", 12325)
