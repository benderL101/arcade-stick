import sys, pcbnew
path = sys.argv[1]
b = pcbnew.LoadBoard(path)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
b.Save(path)
ok = pcbnew.WriteDRCReport(b, path.replace(".kicad_pcb", "_drc.rpt"), pcbnew.EDA_UNITS_MILLIMETRES, True)
print("drc written", ok)
