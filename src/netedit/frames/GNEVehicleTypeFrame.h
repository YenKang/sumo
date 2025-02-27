/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2019 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEVehicleTypeFrame.h
/// @author  Pablo Alvarez Lopez
/// @date    Feb 2018
/// @version $Id$
///
// The Widget for edit Vehicle Type elements
/****************************************************************************/
#ifndef GNEVehicleTypeFrame_h
#define GNEVehicleTypeFrame_h


// ===========================================================================
// included modules
// ===========================================================================
#include "GNEFrame.h"

// ===========================================================================
// class declarations
// ===========================================================================

class GNEVehicle;

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class GNEVehicleTypeFrame
 */
class GNEVehicleTypeFrame : public GNEFrame {

public:

    // ===========================================================================
    // class VehicleTypeSelector
    // ===========================================================================

    class VehicleTypeSelector : protected FXGroupBox {
        /// @brief FOX-declaration
        FXDECLARE(GNEVehicleTypeFrame::VehicleTypeSelector)

    public:
        /// @brief constructor
        VehicleTypeSelector(GNEVehicleTypeFrame* vehicleTypeFrameParent);

        /// @brief destructor
        ~VehicleTypeSelector();

        /// @brief get current Vehicle Type
        GNEDemandElement* getCurrentVehicleType() const;

        /// @brief set current Vehicle Type
        void setCurrentVehicleType(GNEDemandElement* vType);

        /// @brief refresh vehicle type
        void refreshVehicleTypeSelector();

        /// @name FOX-callbacks
        /// @{
        /// @brief Called when the user select another element in ComboBox
        long onCmdSelectItem(FXObject*, FXSelector, void*);
        /// @}

    protected:
        /// @brief FOX needs this
        VehicleTypeSelector() {}

    private:
        /// @brief pointer to Frame Parent
        GNEVehicleTypeFrame* myVehicleTypeFrameParent;

        /// @brief pointer to current vehicle type
        GNEDemandElement* myCurrentVehicleType;

        /// @brief comboBox with the list of elements type
        FXComboBox* myTypeMatchBox;
    };

    // ===========================================================================
    // class VehicleTypeEditor
    // ===========================================================================

    class VehicleTypeEditor : protected FXGroupBox {
        /// @brief FOX-declaration
        FXDECLARE(GNEVehicleTypeFrame::VehicleTypeEditor)

    public:
        /// @brief constructor
        VehicleTypeEditor(GNEVehicleTypeFrame* vehicleTypeFrameParent);

        /// @brief destructor
        ~VehicleTypeEditor();

        /// @brief show VehicleTypeEditor modul
        void showVehicleTypeEditorModul();

        /// @brief hide VehicleTypeEditor box
        void hideVehicleTypeEditorModul();

        /// @brief update VehicleTypeEditor modul
        void refreshVehicleTypeEditorModul();

        /// @name FOX-callbacks
        /// @{
        /// @brief Called when "Vreate Vehicle Type" button is clicked
        long onCmdCreateVehicleType(FXObject*, FXSelector, void*);

        /// @brief Called when "Delete Vehicle Type" button is clicked
        long onCmdDeleteVehicleType(FXObject*, FXSelector, void*);

        /// @brief Called when "Delete Vehicle Type" button is clicked
        long onCmdResetVehicleType(FXObject*, FXSelector, void*);

        /// @brief Called when "Copy Vehicle Type" button is clicked
        long onCmdCopyVehicleType(FXObject*, FXSelector, void*);
        /// @}

    protected:
        /// @brief FOX needs this
        VehicleTypeEditor() {};

    private:
        /// @brief pointer to vehicle type Frame Parent
        GNEVehicleTypeFrame* myVehicleTypeFrameParent;

        /// @brief "create vehicle type" button
        FXButton* myCreateVehicleTypeButton;

        /// @brief "delete vehicle type" button
        FXButton* myDeleteVehicleTypeButton;

        /// @brief "delete default vehicle type" button
        FXButton* myResetDefaultVehicleTypeButton;

        /// @brief "copy vehicle type"
        FXButton* myCopyVehicleTypeButton;
    };

    /**@brief Constructor
     * @brief parent FXHorizontalFrame in which this GNEFrame is placed
     * @brief viewNet viewNet that uses this GNEFrame
     */
    GNEVehicleTypeFrame(FXHorizontalFrame* horizontalFrameParent, GNEViewNet* viewNet);

    /// @brief Destructor
    ~GNEVehicleTypeFrame();

    /// @brief show Frame
    void show();

    /// @brief get vehicle type selector
    VehicleTypeSelector* getVehicleTypeSelector() const;

protected:
    /// @brief function called after set a valid attribute in AttributeCreator/AttributeEditor/GenericParametersEditor/...
    void updateFrameAfterChangeAttribute();

    /// @brief open AttributesCreator extended dialog (used for editing advance attributes of Vehicle Types)
    void openAttributesEditorExtendedDialog();

private:
    /// @brief vehicle type selector
    VehicleTypeSelector* myVehicleTypeSelector;

    /// @brief editorinternal vehicle type attributes
    AttributesEditor* myVehicleTypeAttributesEditor;

    /// @brief modul for open extended attributes dialog
    AttributesEditorExtended* myAttributesEditorExtended;

    /// @brief Vehicle Type editor (Create, copy, etc.)
    VehicleTypeEditor* myVehicleTypeEditor;
};


#endif

/****************************************************************************/
