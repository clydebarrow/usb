mofcomp: htusb.bmf

htusb.bmf: htusb.mof
        mofcomp -B:htusb.bmf htusb.mof
        wmimofck htusb.bmf





