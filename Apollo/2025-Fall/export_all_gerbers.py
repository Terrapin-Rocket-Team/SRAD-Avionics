#!/usr/bin/env python3
"""
Script to export gerbers for all KiCad PCB files in the Apollo folder.
Each project gets its own zip file named after the project.
"""

import os
import sys
import zipfile
from pathlib import Path

try:
    import pcbnew
except ImportError:
    print("ERROR: pcbnew module not found. This script must be run with KiCad's Python interpreter.")
    print("Try running: kicad-cli or check KiCad installation")
    sys.exit(1)


def export_gerbers(pcb_path, output_dir):
    """
    Export gerbers for a given PCB file to the output directory.

    Args:
        pcb_path: Path to the .kicad_pcb file
        output_dir: Directory where gerbers will be saved
    """
    print(f"\nProcessing: {pcb_path}")

    try:
        # Load the board
        board = pcbnew.LoadBoard(str(pcb_path))

        # Create output directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # Set up the plot controller
        plot_controller = pcbnew.PLOT_CONTROLLER(board)
        plot_options = plot_controller.GetPlotOptions()

        # Configure plot options
        plot_options.SetOutputDirectory(output_dir)
        plot_options.SetPlotFrameRef(False)
        plot_options.SetPlotValue(True)
        plot_options.SetPlotReference(True)
        plot_options.SetPlotInvisibleText(False)
        plot_options.SetPlotViaOnMaskLayer(False)
        plot_options.SetExcludeEdgeLayer(True)
        plot_options.SetUseAuxOrigin(False)
        plot_options.SetPlotPadsOnSilkLayer(False)
        plot_options.SetUseGerberProtelExtensions(False)
        plot_options.SetCreateGerberJobFile(True)
        plot_options.SetSubtractMaskFromSilk(False)
        plot_options.SetPlotViaOnMaskLayer(False)

        # Set Gerber precision
        plot_options.SetGerberPrecision(6)

        # Define layers to plot
        layer_info = [
            (pcbnew.F_Cu, pcbnew.PLOT_FORMAT_GERBER, "F_Cu"),
            (pcbnew.B_Cu, pcbnew.PLOT_FORMAT_GERBER, "B_Cu"),
            (pcbnew.F_SilkS, pcbnew.PLOT_FORMAT_GERBER, "F_SilkS"),
            (pcbnew.B_SilkS, pcbnew.PLOT_FORMAT_GERBER, "B_SilkS"),
            (pcbnew.F_Mask, pcbnew.PLOT_FORMAT_GERBER, "F_Mask"),
            (pcbnew.B_Mask, pcbnew.PLOT_FORMAT_GERBER, "B_Mask"),
            (pcbnew.F_Paste, pcbnew.PLOT_FORMAT_GERBER, "F_Paste"),
            (pcbnew.B_Paste, pcbnew.PLOT_FORMAT_GERBER, "B_Paste"),
            (pcbnew.Edge_Cuts, pcbnew.PLOT_FORMAT_GERBER, "Edge_Cuts"),
        ]

        # Add inner copper layers if they exist
        for i in range(1, board.GetCopperLayerCount() - 1):
            layer_name = f"In{i}_Cu"
            layer_info.append((pcbnew.In1_Cu + (i-1), pcbnew.PLOT_FORMAT_GERBER, layer_name))

        # Plot each layer
        for layer_id, plot_format, layer_name in layer_info:
            plot_controller.SetLayer(layer_id)
            plot_controller.OpenPlotfile(layer_name, plot_format, "")
            plot_controller.PlotLayer()

        # Close the plot controller
        plot_controller.ClosePlot()

        # Generate drill files
        drill_writer = pcbnew.EXCELLON_WRITER(board)
        drill_writer.SetOptions(
            False,  # aMirror
            False,  # aMinimalHeader
            pcbnew.VECTOR2I(0, 0),  # aOffset
            False   # aMerge_PTH_NPTH
        )
        drill_writer.SetFormat(False)  # Use mm, not inches
        drill_writer.CreateDrillandMapFilesSet(output_dir, True, False)

        print(f"  ✓ Gerbers exported to: {output_dir}")
        return True

    except Exception as e:
        print(f"  ✗ ERROR exporting {pcb_path}: {str(e)}")
        return False


def create_zip(source_dir, zip_path):
    """Create a zip file from all files in source_dir."""
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(source_dir):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, source_dir)
                zipf.write(file_path, arcname)
    print(f"  ✓ Created zip: {zip_path}")


def main():
    # Base paths
    script_dir = Path(__file__).parent
    apollo_dir = script_dir
    gerbers_master_dir = apollo_dir / "Gerbers"

    # Find all .kicad_pcb files
    pcb_files = list(apollo_dir.rglob("*.kicad_pcb"))

    # Filter out files in the Gerbers directory itself
    pcb_files = [f for f in pcb_files if "Gerbers" not in str(f)]

    print(f"Found {len(pcb_files)} PCB files to process")
    print("=" * 60)

    success_count = 0
    failed_count = 0

    for pcb_file in pcb_files:
        # Get project name (filename without extension)
        project_name = pcb_file.stem

        # Create temporary directory for gerbers
        temp_gerber_dir = gerbers_master_dir / f"{project_name}_temp"

        # Export gerbers
        if export_gerbers(pcb_file, str(temp_gerber_dir)):
            # Create zip file
            zip_path = gerbers_master_dir / f"{project_name}_gerbers.zip"
            create_zip(temp_gerber_dir, zip_path)

            # Clean up temporary directory
            import shutil
            shutil.rmtree(temp_gerber_dir)

            success_count += 1
        else:
            failed_count += 1

    print("\n" + "=" * 60)
    print(f"Export complete!")
    print(f"  Successful: {success_count}")
    print(f"  Failed: {failed_count}")
    print(f"  Output directory: {gerbers_master_dir}")


if __name__ == "__main__":
    main()
