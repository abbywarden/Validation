import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Run3_cff import Run3

process = cms.Process('VALIDATION',Run3)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load("Validation.MuonCSCDigis.cscDigiValidation_cfi")
process.load('Configuration.StandardSequences.DQMSaverAtRunEnd_cff')
process.load('DQMOffline.Configuration.DQMOfflineMC_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(25),
    #input = cms.untracked.int32(-100),
    output = cms.optional.untracked.allowed(cms.int32,cms.PSet)
)

# Input source
#currently the two files gives 9,000 events;
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
        "/store/relval/CMSSW_11_3_0_pre5/RelValSingleMuPt10/GEN-SIM-DIGI-RAW/113X_mcRun3_2021_realistic_v7-v1/00000/e1f09711-5c3a-4f54-99e4-d628abba5729.root",
        "/store/relval/CMSSW_11_3_0_pre5/RelValSingleMuPt10/GEN-SIM-DIGI-RAW/113X_mcRun3_2021_realistic_v7-v1/00000/e334818f-da40-4d02-a46d-7ccf1feb4725.root"
    ),
    secondaryFileNames = cms.untracked.vstring(
    )
)

# Other statements
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2021_realistic', '')

process.DQMoutput = cms.OutputModule("DQMRootOutputModule",
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string('DQMIO'),
        filterName = cms.untracked.string('')
    ),
    fileName = cms.untracked.string('file:step3_inDQM.root'),
    outputCommands = process.DQMEventContent.outputCommands,
    splitLevel = cms.untracked.int32(0)
)

# Path and EndPath definitions
process.validation_step = cms.Path(process.mix * process.cscDigiValidation)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.DQMoutput_step = cms.EndPath(process.DQMoutput)

# Schedule definition
process.schedule = cms.Schedule(
    process.validation_step,
    process.endjob_step,
    process.DQMoutput_step
)
