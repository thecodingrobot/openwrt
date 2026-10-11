/*
 * omci/me_defs.c — managed-entity schema tables
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * Attribute names, ordering, sizes and mask bit assignments follow
 * ITU-T G.988; cross-checked against the Apache-2.0 OpenOMCI
 * omci-lib-go generated definitions (attribution: ONF/OpenOMCI).
 * Generated table — edit gen script, not by hand.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "me_defs.h"

#include <stddef.h>
#include <string.h>


static const struct omci_attr_def attrs_onu_data[] = {
    { "MibDataSync", A_U8, 1, 0x8000, AF_WRITE },
};
const struct omci_class_def omci_class_onu_data = {
    2, "ONU Data", attrs_onu_data, 1, 0
};

static const struct omci_attr_def attrs_cardholder[] = {
    { "ActualPlugInUnitType", A_U8, 1, 0x8000, 0 },
    { "ExpectedPlugInUnitType", A_U8, 1, 0x4000, AF_WRITE },
    { "ExpectedPortCount", A_U8, 1, 0x2000, AF_WRITE },
    { "ExpectedEquipmentId", A_STR, 20, 0x1000, AF_WRITE },
    { "ActualEquipmentId", A_STR, 20, 0x0800, 0 },
    { "ProtectionProfilePointer", A_U8, 1, 0x0400, 0 },
    { "InvokeProtectionSwitch", A_U8, 1, 0x0200, AF_WRITE },
    { "AlarmReportingControl", A_U8, 1, 0x0100, AF_WRITE },
    { "ArcInterval", A_U8, 1, 0x0080, AF_WRITE },
};
const struct omci_class_def omci_class_cardholder = {
    5, "Cardholder", attrs_cardholder, 9, 0
};

static const struct omci_attr_def attrs_circuit_pack[] = {
    { "Type", A_U8, 1, 0x8000, AF_SBC },
    { "NumberOfPorts", A_U8, 1, 0x4000, 0 },
    { "SerialNumber", A_BYTES, 8, 0x2000, 0 },
    { "Version", A_BYTES, 14, 0x1000, 0 },
    { "VendorId", A_STR, 4, 0x0800, 0 },
    { "AdministrativeState", A_U8, 1, 0x0400, AF_WRITE },
    { "OperationalState", A_U8, 1, 0x0200, 0 },
    { "BridgedOrIpInd", A_U8, 1, 0x0100, AF_WRITE },
    { "EquipmentId", A_STR, 20, 0x0080, 0 },
    { "CardConfiguration", A_U8, 1, 0x0040, AF_SBC|AF_WRITE },
    { "TotalTContBufferNumber", A_U8, 1, 0x0020, 0 },
    { "TotalPriorityQueueNumber", A_U8, 1, 0x0010, 0 },
    { "TotalTrafficSchedulerNumber", A_U8, 1, 0x0008, 0 },
    { "PowerShedOverride", A_U32, 4, 0x0004, AF_WRITE },
};
const struct omci_class_def omci_class_circuit_pack = {
    6, "Circuit pack", attrs_circuit_pack, 14, 0
};

static const struct omci_attr_def attrs_software_image[] = {
    { "Version", A_STR, 14, 0x8000, 0 },
    { "IsCommitted", A_U8, 1, 0x4000, 0 },
    { "IsActive", A_U8, 1, 0x2000, 0 },
    { "IsValid", A_U8, 1, 0x1000, 0 },
    { "ProductCode", A_BYTES, 25, 0x0800, 0 },
    { "ImageHash", A_STR, 16, 0x0400, 0 },
};
const struct omci_class_def omci_class_software_image = {
    7, "Software image", attrs_software_image, 6, 0
};

static const struct omci_attr_def attrs_pptp_ethernet_uni[] = {
    { "ExpectedType", A_U8, 1, 0x8000, AF_WRITE },
    { "SensedType", A_U8, 1, 0x4000, 0 },
    { "AutoDetectionConfiguration", A_U8, 1, 0x2000, AF_WRITE },
    { "EthernetLoopbackConfiguration", A_U8, 1, 0x1000, AF_WRITE },
    { "AdministrativeState", A_U8, 1, 0x0800, AF_WRITE },
    { "OperationalState", A_U8, 1, 0x0400, 0 },
    { "ConfigurationInd", A_U8, 1, 0x0200, 0 },
    { "MaxFrameSize", A_U16, 2, 0x0100, AF_WRITE },
    { "DteOrDceInd", A_U8, 1, 0x0080, AF_WRITE },
    { "PauseTime", A_U16, 2, 0x0040, AF_WRITE },
    { "BridgedOrIpInd", A_U8, 1, 0x0020, AF_WRITE },
    { "Arc", A_U8, 1, 0x0010, AF_WRITE },
    { "ArcInterval", A_U8, 1, 0x0008, AF_WRITE },
    { "PppoeFilter", A_U8, 1, 0x0004, AF_WRITE },
    { "PowerControl", A_U8, 1, 0x0002, AF_WRITE },
};
const struct omci_class_def omci_class_pptp_ethernet_uni = {
    11, "PPTP Ethernet UNI", attrs_pptp_ethernet_uni, 15, 0
};

static const struct omci_attr_def attrs_mac_bridge_service_profile[] = {
    { "SpanningTreeInd", A_U8, 1, 0x8000, AF_SBC|AF_WRITE },
    { "LearningInd", A_U8, 1, 0x4000, AF_SBC|AF_WRITE },
    { "PortBridgingInd", A_U8, 1, 0x2000, AF_SBC|AF_WRITE },
    { "Priority", A_U16, 2, 0x1000, AF_SBC|AF_WRITE },
    { "MaxAge", A_U16, 2, 0x0800, AF_SBC|AF_WRITE },
    { "HelloTime", A_U16, 2, 0x0400, AF_SBC|AF_WRITE },
    { "ForwardDelay", A_U16, 2, 0x0200, AF_SBC|AF_WRITE },
    { "UnknownMacAddressDiscard", A_U8, 1, 0x0100, AF_SBC|AF_WRITE },
    { "MacLearningDepth", A_U8, 1, 0x0080, AF_SBC|AF_WRITE },
    { "DynamicFilteringAgeingTime", A_U32, 4, 0x0040, AF_SBC|AF_WRITE },
};
const struct omci_class_def omci_class_mac_bridge_service_profile = {
    45, "MAC bridge service profile", attrs_mac_bridge_service_profile, 10, 0
};

static const struct omci_attr_def attrs_mac_bridge_port_config_data[] = {
    { "BridgeIdPointer", A_U16, 2, 0x8000, AF_SBC|AF_WRITE },
    { "PortNum", A_U8, 1, 0x4000, AF_SBC|AF_WRITE },
    { "TpType", A_U8, 1, 0x2000, AF_SBC|AF_WRITE },
    { "TpPointer", A_U16, 2, 0x1000, AF_SBC|AF_WRITE },
    { "PortPriority", A_U16, 2, 0x0800, AF_SBC|AF_WRITE },
    { "PortPathCost", A_U16, 2, 0x0400, AF_SBC|AF_WRITE },
    { "PortSpanningTreeInd", A_U8, 1, 0x0200, AF_SBC|AF_WRITE },
    { "Deprecated1", A_U8, 1, 0x0100, AF_SBC|AF_WRITE },
    { "Deprecated2", A_U8, 1, 0x0080, AF_SBC|AF_WRITE },
    { "PortMacAddress", A_BYTES, 6, 0x0040, 0 },
    { "OutboundTdPointer", A_U16, 2, 0x0020, AF_WRITE },
    { "InboundTdPointer", A_U16, 2, 0x0010, AF_WRITE },
    { "MacLearningDepth", A_U8, 1, 0x0008, AF_SBC|AF_WRITE },
    { "LaspIdPointer", A_U16, 2, 0x0004, AF_SBC|AF_WRITE },
};
const struct omci_class_def omci_class_mac_bridge_port_config_data = {
    47, "MAC bridge port config data", attrs_mac_bridge_port_config_data, 14, 0
};

static const struct omci_attr_def attrs_vlan_tagging_filter_data[] = {
    { "VlanFilterList", A_BYTES, 24, 0x8000, AF_SBC|AF_WRITE },
    { "ForwardOperation", A_U8, 1, 0x4000, AF_SBC|AF_WRITE },
    { "NumberOfEntries", A_U8, 1, 0x2000, AF_SBC|AF_WRITE },
};
const struct omci_class_def omci_class_vlan_tagging_filter_data = {
    84, "VLAN tagging filter data", attrs_vlan_tagging_filter_data, 3, 0
};

static const struct omci_attr_def attrs_ieee_802_1p_mapper_service_profile[] = {
    { "TpPointer", A_U16, 2, 0x8000, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority0", A_U16, 2, 0x4000, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority1", A_U16, 2, 0x2000, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority2", A_U16, 2, 0x1000, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority3", A_U16, 2, 0x0800, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority4", A_U16, 2, 0x0400, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority5", A_U16, 2, 0x0200, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority6", A_U16, 2, 0x0100, AF_SBC|AF_WRITE },
    { "InterworkTpPointerForPBitPriority7", A_U16, 2, 0x0080, AF_SBC|AF_WRITE },
    { "UnmarkedFrameOption", A_U8, 1, 0x0040, AF_SBC|AF_WRITE },
    { "DscpToPBitMapping", A_BYTES, 24, 0x0020, AF_WRITE },
    { "DefaultPBitAssumption", A_U8, 1, 0x0010, AF_SBC|AF_WRITE },
    { "TpType", A_U8, 1, 0x0008, AF_SBC|AF_WRITE },
};
const struct omci_class_def omci_class_ieee_802_1p_mapper_service_profile = {
    130, "IEEE 802.1p mapper service profile", attrs_ieee_802_1p_mapper_service_profile, 13, 0
};

static const struct omci_attr_def attrs_olt_g[] = {
    { "OltVendorId", A_STR, 4, 0x8000, AF_WRITE },
    { "EquipmentId", A_STR, 20, 0x4000, AF_WRITE },
    { "Version", A_STR, 14, 0x2000, AF_WRITE },
    { "TimeOfDayInformation", A_BYTES, 14, 0x1000, AF_WRITE },
};
const struct omci_class_def omci_class_olt_g = {
    131, "OLT-G", attrs_olt_g, 4, 0
};

static const struct omci_attr_def attrs_onu_remote_debug[] = {
    { "CommandFormat", A_U8, 1, 0x8000, 0 },
    { "Command", A_BYTES, 25, 0x4000, AF_WRITE },
};
const struct omci_class_def omci_class_onu_remote_debug = {
    158, "ONU remote debug", attrs_onu_remote_debug, 2, 0
};

static const struct omci_attr_def attrs_ext_vlan_tagging_oper_config_data[] = {
    { "AssociationType", A_U8, 1, 0x8000, AF_SBC|AF_WRITE },
    { "ReceivedFrameVlanTaggingOperationTableMaxSize", A_U16, 2, 0x4000, 0 },
    { "InputTpid", A_U16, 2, 0x2000, AF_WRITE },
    { "OutputTpid", A_U16, 2, 0x1000, AF_WRITE },
    { "DownstreamMode", A_U8, 1, 0x0800, AF_WRITE },
    { "AssociatedMePointer", A_U16, 2, 0x0200, AF_SBC|AF_WRITE },
    { "DscpToPBitMapping", A_BYTES, 24, 0x0100, AF_WRITE },
    { "EnhancedMode", A_U8, 1, 0x0080, AF_SBC },
};
const struct omci_class_def omci_class_ext_vlan_tagging_oper_config_data = {
    171, "Ext VLAN tagging oper config data", attrs_ext_vlan_tagging_oper_config_data, 8, 0
};

static const struct omci_attr_def attrs_onu_g[] = {
    { "VendorId", A_STR, 4, 0x8000, 0 },
    { "Version", A_STR, 14, 0x4000, 0 },
    { "SerialNumber", A_BYTES, 8, 0x2000, 0 },
    { "TrafficManagementOption", A_U8, 1, 0x1000, 0 },
    { "Deprecated", A_U8, 1, 0x0800, 0 },
    { "BatteryBackup", A_U8, 1, 0x0400, AF_WRITE },
    { "AdministrativeState", A_U8, 1, 0x0200, AF_WRITE },
    { "OperationalState", A_U8, 1, 0x0100, 0 },
    { "OnuSurvivalTime", A_U8, 1, 0x0080, 0 },
    { "LogicalOnuId", A_BYTES, 24, 0x0040, 0 },
    { "LogicalPassword", A_BYTES, 12, 0x0020, 0 },
    { "CredentialsStatus", A_U8, 1, 0x0010, AF_WRITE },
    { "ExtendedTcLayerOptions", A_U16, 2, 0x0008, 0 },
};
const struct omci_class_def omci_class_onu_g = {
    256, "ONU-G", attrs_onu_g, 13, 0
};

static const struct omci_attr_def attrs_onu2_g[] = {
    { "EquipmentId", A_STR, 20, 0x8000, 0 },
    { "OpticalNetworkUnitManagementAndControlChannelOmccVersion", A_U8, 1, 0x4000, 0 },
    { "VendorProductCode", A_U16, 2, 0x2000, 0 },
    { "SecurityCapability", A_U8, 1, 0x1000, 0 },
    { "SecurityMode", A_U8, 1, 0x0800, AF_WRITE },
    { "TotalPriorityQueueNumber", A_U16, 2, 0x0400, 0 },
    { "TotalTrafficSchedulerNumber", A_U8, 1, 0x0200, 0 },
    { "Deprecated", A_U8, 1, 0x0100, 0 },
    { "TotalGemPortIdNumber", A_U16, 2, 0x0080, 0 },
    { "Sysuptime", A_U32, 4, 0x0040, 0 },
    { "ConnectivityCapability", A_U16, 2, 0x0020, 0 },
    { "CurrentConnectivityMode", A_U8, 1, 0x0010, AF_WRITE },
    { "QualityOfServiceQosConfigurationFlexibility", A_U16, 2, 0x0008, 0 },
    { "PriorityQueueScaleFactor", A_U16, 2, 0x0004, AF_WRITE },
};
const struct omci_class_def omci_class_onu2_g = {
    257, "ONU2-G", attrs_onu2_g, 14, 0
};

static const struct omci_attr_def attrs_t_cont[] = {
    { "AllocId", A_U16, 2, 0x8000, AF_WRITE },
    { "Deprecated", A_U8, 1, 0x4000, 0 },
    { "Policy", A_U8, 1, 0x2000, AF_WRITE },
};
const struct omci_class_def omci_class_t_cont = {
    262, "T-CONT", attrs_t_cont, 3, 0
};

static const struct omci_attr_def attrs_ani_g[] = {
    { "SrIndication", A_U8, 1, 0x8000, 0 },
    { "TotalTcontNumber", A_U16, 2, 0x4000, 0 },
    { "GemBlockLength", A_U16, 2, 0x2000, AF_WRITE },
    { "PiggybackDbaReporting", A_U8, 1, 0x1000, 0 },
    { "Deprecated", A_U8, 1, 0x0800, 0 },
    { "SignalFailThreshold", A_U8, 1, 0x0400, AF_WRITE },
    { "SignalDegradeThreshold", A_U8, 1, 0x0200, AF_WRITE },
    { "Arc", A_U8, 1, 0x0100, AF_WRITE },
    { "ArcInterval", A_U8, 1, 0x0080, AF_WRITE },
    { "OpticalSignalLevel", A_U16, 2, 0x0040, 0 },
    { "LowerOpticalThreshold", A_U8, 1, 0x0020, AF_WRITE },
    { "UpperOpticalThreshold", A_U8, 1, 0x0010, AF_WRITE },
    { "OnuResponseTime", A_U16, 2, 0x0008, 0 },
    { "TransmitOpticalLevel", A_U16, 2, 0x0004, 0 },
    { "LowerTransmitPowerThreshold", A_U8, 1, 0x0002, AF_WRITE },
    { "UpperTransmitPowerThreshold", A_U8, 1, 0x0001, AF_WRITE },
};
const struct omci_class_def omci_class_ani_g = {
    263, "ANI-G", attrs_ani_g, 16, 0
};

static const struct omci_attr_def attrs_uni_g[] = {
    { "Deprecated", A_U16, 2, 0x8000, AF_WRITE },
    { "AdministrativeState", A_U8, 1, 0x4000, AF_WRITE },
    { "ManagementCapability", A_U8, 1, 0x2000, 0 },
    { "NonOmciManagementIdentifier", A_U16, 2, 0x1000, AF_WRITE },
    { "RelayAgentOptions", A_U16, 2, 0x0800, AF_WRITE },
};
const struct omci_class_def omci_class_uni_g = {
    264, "UNI-G", attrs_uni_g, 5, 0
};

static const struct omci_attr_def attrs_traffic_descriptor[] = {
    { "Cir", A_U32, 4, 0x8000, AF_SBC|AF_WRITE },
    { "Pir", A_U32, 4, 0x4000, AF_SBC|AF_WRITE },
    { "Cbs", A_U32, 4, 0x2000, AF_SBC|AF_WRITE },
    { "Pbs", A_U32, 4, 0x1000, AF_SBC|AF_WRITE },
    { "ColourMode", A_U8, 1, 0x0800, AF_SBC|AF_WRITE },
    { "IngressColourMarking", A_U8, 1, 0x0400, AF_SBC|AF_WRITE },
    { "EgressColourMarking", A_U8, 1, 0x0200, AF_SBC|AF_WRITE },
    { "MeterType", A_U8, 1, 0x0100, AF_SBC },
};
const struct omci_class_def omci_class_traffic_descriptor = {
    266, "Traffic descriptor", attrs_traffic_descriptor, 8, 0
};

static const struct omci_attr_def attrs_gem_port_network_ctp[] = {
    { "PortId", A_U16, 2, 0x8000, AF_SBC|AF_WRITE },
    { "TContPointer", A_U16, 2, 0x4000, AF_SBC|AF_WRITE },
    { "Direction", A_U8, 1, 0x2000, AF_SBC|AF_WRITE },
    { "TrafficManagementPointerForUpstream", A_U16, 2, 0x1000, AF_SBC|AF_WRITE },
    { "TrafficDescriptorProfilePointerForUpstream", A_U16, 2, 0x0800, AF_SBC|AF_WRITE },
    { "UniCounter", A_U8, 1, 0x0400, 0 },
    { "PriorityQueuePointerForDownStream", A_U16, 2, 0x0200, AF_SBC|AF_WRITE },
    { "EncryptionState", A_U8, 1, 0x0100, 0 },
    { "TrafficDescriptorProfilePointerForDownstream", A_U16, 2, 0x0080, AF_SBC|AF_WRITE },
    { "EncryptionKeyRing", A_U8, 1, 0x0040, AF_SBC|AF_WRITE },
};
const struct omci_class_def omci_class_gem_port_network_ctp = {
    268, "GEM port network CTP", attrs_gem_port_network_ctp, 10, 0
};

static const struct omci_attr_def attrs_priority_queue[] = {
    { "QueueConfigurationOption", A_U8, 1, 0x8000, 0 },
    { "MaximumQueueSize", A_U16, 2, 0x4000, 0 },
    { "AllocatedQueueSize", A_U16, 2, 0x2000, AF_WRITE },
    { "DiscardBlockCounterResetInterval", A_U16, 2, 0x1000, AF_WRITE },
    { "ThresholdValueForDiscardedBlocksDueToBufferOverflow", A_U16, 2, 0x0800, AF_WRITE },
    { "RelatedPort", A_U32, 4, 0x0400, AF_WRITE },
    { "TrafficSchedulerPointer", A_U16, 2, 0x0200, AF_WRITE },
    { "Weight", A_U8, 1, 0x0100, AF_WRITE },
    { "BackPressureOperation", A_U16, 2, 0x0080, AF_WRITE },
    { "BackPressureTime", A_U32, 4, 0x0040, AF_WRITE },
    { "BackPressureOccurQueueThreshold", A_U16, 2, 0x0020, AF_WRITE },
    { "BackPressureClearQueueThreshold", A_U16, 2, 0x0010, AF_WRITE },
    { "PacketDropQueueThresholds", A_TABLE, 0, 0x0008, AF_WRITE|AF_TABLE },
    { "PacketDropMaxP", A_U16, 2, 0x0004, AF_WRITE },
    { "QueueDropWQ", A_U8, 1, 0x0002, AF_WRITE },
    { "DropPrecedenceColourMarking", A_U8, 1, 0x0001, AF_WRITE },
};
const struct omci_class_def omci_class_priority_queue = {
    277, "Priority queue", attrs_priority_queue, 16, 0
};

static const struct omci_attr_def attrs_traffic_scheduler[] = {
    { "TContPointer", A_U16, 2, 0x8000, AF_WRITE },
    { "TrafficSchedulerPointer", A_U16, 2, 0x4000, 0 },
    { "Policy", A_U8, 1, 0x2000, AF_WRITE },
    { "PriorityWeight", A_U8, 1, 0x1000, AF_WRITE },
};
const struct omci_class_def omci_class_traffic_scheduler = {
    278, "Traffic scheduler", attrs_traffic_scheduler, 4, 0
};

static const struct omci_attr_def attrs_omci[] = {
};
const struct omci_class_def omci_class_omci = {
    287, "OMCI", attrs_omci, 0, 0
};

static const struct omci_attr_def attrs_onu_dynamic_power_mgmt_control[] = {
    { "PowerReductionManagementCapability", A_U8, 1, 0x8000, 0 },
    { "PowerReductionManagementMode", A_U8, 1, 0x4000, AF_WRITE },
    { "Itransinit", A_U16, 2, 0x2000, 0 },
    { "Itxinit", A_U16, 2, 0x1000, 0 },
    { "MaximumSleepInterval", A_U32, 4, 0x0800, AF_WRITE },
    { "MaximumReceiverOffInterval", A_U32, 4, 0x0400, AF_WRITE },
    { "MinimumAwareInterval", A_U32, 4, 0x0200, AF_WRITE },
    { "MinimumActiveHeldInterval", A_U16, 2, 0x0100, AF_WRITE },
    { "MaximumSleepIntervalExtension", A_TABLE, 0, 0x0080, AF_WRITE|AF_TABLE },
    { "EthernetPassiveOpticalNetworkEponCapabilityExtension", A_U8, 1, 0x0040, 0 },
    { "EponSetupExtension", A_U8, 1, 0x0020, AF_WRITE },
    { "MissingConsecutiveBurstsThreshold", A_U32, 4, 0x0010, AF_WRITE },
};
const struct omci_class_def omci_class_onu_dynamic_power_mgmt_control = {
    336, "ONU dynamic power mgmt control", attrs_onu_dynamic_power_mgmt_control, 12, 0
};

const struct omci_class_def *const omci_classes[] = {
    &omci_class_onu_data,
    &omci_class_cardholder,
    &omci_class_circuit_pack,
    &omci_class_software_image,
    &omci_class_pptp_ethernet_uni,
    &omci_class_mac_bridge_service_profile,
    &omci_class_mac_bridge_port_config_data,
    &omci_class_vlan_tagging_filter_data,
    &omci_class_ieee_802_1p_mapper_service_profile,
    &omci_class_olt_g,
    &omci_class_onu_remote_debug,
    &omci_class_ext_vlan_tagging_oper_config_data,
    &omci_class_onu_g,
    &omci_class_onu2_g,
    &omci_class_t_cont,
    &omci_class_ani_g,
    &omci_class_uni_g,
    &omci_class_traffic_descriptor,
    &omci_class_gem_port_network_ctp,
    &omci_class_priority_queue,
    &omci_class_traffic_scheduler,
    &omci_class_omci,
    &omci_class_onu_dynamic_power_mgmt_control,
};
const unsigned omci_class_count = 23;

/* ---- manual overrides for table slot sizes (stick .ini layout) ---------- */
/* The stock MIB seed files allocate fixed slots for table attributes,
 * e.g. ME 277 packet-drop thresholds use 8 bytes
 * ("\x15\x56\x15\x56\x15\x56\x15\x56" in the reference profile). */
struct tbl_fix { uint16_t cls; unsigned attr_no; uint16_t size; };
static const struct tbl_fix tbl_fixes[] = {
    { 277, 13, 8 },   /* priority queue: packet drop queue thresholds */
};
static const size_t tbl_fix_count = sizeof(tbl_fixes) / sizeof(tbl_fixes[0]);

/* ---- lookup + helpers (generated data above) --------------------------- */

#include <string.h>

const struct omci_class_def *omci_class_find(uint16_t class_id)
{
    unsigned i;
    for (i = 0; i < omci_class_count; i++)
        if (omci_classes[i]->class_id == class_id)
            return omci_classes[i];
    return NULL;
}

const struct omci_attr_def *omci_attr(const struct omci_class_def *cd,
                                      unsigned attr_no)
{
    if (!cd || attr_no == 0 || attr_no > cd->num_attrs)
        return NULL;
    return &cd->attrs[attr_no - 1];
}

int omci_attr_size(const struct omci_class_def *cd, unsigned attr_no)
{
    const struct omci_attr_def *ad = omci_attr(cd, attr_no);
    size_t i;
    if (!ad)
        return -1;
    if (ad->type == A_TABLE) {
        for (i = 0; i < tbl_fix_count; i++)
            if (tbl_fixes[i].cls == cd->class_id &&
                tbl_fixes[i].attr_no == attr_no)
                return tbl_fixes[i].size;
        return 0;   /* unsized table: no storage */
    }
    return ad->size;
}

int omci_attr_offset(const struct omci_class_def *cd, unsigned attr_no)
{
    int off = 0;
    unsigned n;
    if (!cd || attr_no == 0 || attr_no > cd->num_attrs)
        return -1;
    for (n = 1; n < attr_no; n++) {
        int sz = omci_attr_size(cd, n);
        if (sz < 0)
            return -1;
        off += sz;
    }
    return off;
}

unsigned omci_attr_no_for_mask(const struct omci_class_def *cd,
                               uint16_t mask_bit)
{
    unsigned n;
    if (!cd)
        return 0;
    for (n = 1; n <= cd->num_attrs; n++)
        if (cd->attrs[n - 1].mask == mask_bit)
            return n;
    return 0;
}

uint16_t omci_class_upload_mask(const struct omci_class_def *cd)
{
    uint16_t mask = 0;
    unsigned n;
    if (!cd)
        return 0;
    for (n = 1; n <= cd->num_attrs; n++) {
        const struct omci_attr_def *ad = &cd->attrs[n - 1];
        if (ad->flags & AF_NOUP)
            continue;
        if (ad->type == A_TABLE && omci_attr_size(cd, n) == 0)
            continue;   /* unsized tables are not reported */
        mask |= ad->mask;
    }
    return mask;
}

int omci_class_attr_area_len(const struct omci_class_def *cd)
{
    if (!cd || cd->num_attrs == 0)
        return 0;
    return omci_attr_offset(cd, cd->num_attrs) +
           omci_attr_size(cd, cd->num_attrs);
}

int omci_mask_packed_len(const struct omci_class_def *cd, uint16_t mask)
{
    int total = 0;
    unsigned n;

    if (!cd)
        return -1;
    for (n = 1; n <= cd->num_attrs; n++) {
        const struct omci_attr_def *ad = &cd->attrs[n - 1];
        int sz;
        if (!(mask & ad->mask))
            continue;
        sz = omci_attr_size(cd, n);
        if (sz < 0)
            return -1;
        total += sz;
    }
    return total;
}

void omci_attr_flags_str(const struct omci_attr_def *ad, char *dst)
{
    /* 11-character permission string, flag order R W S A V E U T P N Y.
     * Approximate until [HW] capture confirms the exact stock ordering
     * (see specs/interface-b.md section 3). */
    strcpy(dst, "-----------");
    dst[0] = 'R';
    if (ad->flags & AF_WRITE)
        dst[1] = 'W';
    if (ad->flags & AF_SBC)
        dst[2] = 'S';
    if (ad->flags & AF_TABLE)
        dst[7] = 'T';
}

const char *omci_attr_type_str(const struct omci_attr_def *ad)
{
    switch (ad->type) {
    case A_U8:
    case A_U16:
    case A_U32:     return "UINT";
    case A_STR:     return "STR";
    case A_BYTES:   return "STR";
    case A_TABLE:   return "TBL";
    default:        return "?";
    }
}
