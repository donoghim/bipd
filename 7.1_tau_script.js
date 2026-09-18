import TestCase from '#libs/TestCase';
import Entity from '#libs/Entity';
import EntityConfigMmeLGU from '#scripts/LGU/config/EntityConfigMmeLGU';
import EntityConfigEnbLGU from '#scripts/LGU/config/EntityConfigEnbLGU';
// import EntityConfigImsLGU from '#scripts/LGU/config/EntityConfigImsLGU';
import { defaultLTECellCfg, defaultNBCellCfg, defaultNRCellCfg } from '#scripts/LGU/config/cellDefault';
import Logger from '#libs/Logger';
import { waitMS } from '#libs/Utils';
import { doTree } from '#libs/Parser';
import { checkEmmAttachProcedure, checkEmmTauProcedure } from './CheckProcedure.js';
import { config as CONFIG } from '#config'

const logger = new Logger();

export class TestItem extends TestCase {
  id = 701;
  title = '(TC 7.1) TAU Normal';
  description = 'Check NB-IoT TAU Normal procedure';

  async run(options) {
    await this.log('DUT turn off RF');
    await this.adapter.setRF(0);

    await this.adapter.setRF(4);
    await waitMS(CONFIG.WAIT_FOR_SIM_READY_MS);
    this.dutInfo = await this.adapter.getDutInformation();

    // Initialize entity config
    const mmeConfig = new EntityConfigMmeLGU();
    const enbConfig = new EntityConfigEnbLGU();
    // const imsConfig = new EntityConfigImsLGU();

    /* Set LTE cells for TAU trigger (same method as LTE 6.1) */
    enbConfig.setCellList({
      cell_list: [
        {
          rf_port: 0,
          ...defaultLTECellCfg,
          // dl_earfcn: 2590, 
          cell_id: 1,
          n_id_cell: 1,
          tac: 200,
          root_sequence_index: 204,
          n_antenna_dl: 1,
          n_antenna_ul: 1,
          ncell_list: [
            {
              rat: 'eutra',
              n_id_cell: 2,
              cell_id: 2,
              tac: 300,
              dl_earfcn: defaultLTECellCfg.dl_earfcn,
            }
          ],
          // inactivity_timer: 1000,
          // rrc_reject_waitTime: 1,
          // rrc_release_waitTime: 1,
          // cell_gain: -200,
        },
        {
          rf_port: 1,
          // rf_port: 0,
          ...defaultLTECellCfg,
          // dl_earfcn: 2590, 
          // dl_earfcn: 100, 
          cell_id: 2,
          n_id_cell: 2,
          tac: 300,
          root_sequence_index: 16,
          n_antenna_dl: 1,
          n_antenna_ul: 1,
          ncell_list: [
            {
              rat: 'eutra',
              n_id_cell: 1,
              cell_id: 1,
              tac: 200,
              dl_earfcn: defaultLTECellCfg.dl_earfcn,
            }
          ],
          // inactivity_timer: 1000,
          // rrc_reject_waitTime: 1,
          // rrc_release_waitTime: 1,
          // cell_gain: -200,
        },
      ],
      nb_cell_list: [
        {
          ...defaultNBCellCfg,
          rf_port: 0,
          cell_id: 11,
          tac: 11, /* SIB1.trackingAreaCode */
          n_id_cell: 11,
          base_cell_id: 1,
          // dl_prb: 14,
          // ul_prb: 14,
          n_antenna_dl: 1,
          n_antenna_ul: 1,
          ncell_list: [
            {
              rat: 'eutra',
              cell_id: 12,
              tac: 12,
              dl_earfcn: defaultLTECellCfg.dl_earfcn,
            }
          ],
          // cell_gain: -200,
        },
        {
          ...defaultNBCellCfg,
          rf_port: 1,
          // rf_port: 0,
          cell_id: 12,
          tac: 12, /* SIB1.trackingAreaCode */
          n_id_cell: 12,
          base_cell_id: 2,
          // dl_prb: 40, // 14, // 35, /* DL PRB number in the base LTE cell */
          // ul_prb: 40, // 14, // 35, /* UL PRB number in the base LTE cell */
          // dl_prb: 35,
          // ul_prb: 35,
          n_antenna_dl: 1,
          n_antenna_ul: 1,
          ncell_list: [
            {
              rat: 'eutra',
              cell_id: 11,
              tac: 11,
              dl_earfcn: defaultLTECellCfg.dl_earfcn,
              // dl_earfcn: 100,
            }
          ],
          // cell_gain: -200,
        },
      ],
      nr_cell_list: [
      ],
    });

    // SIB3/SIB5 
    enbConfig.nb_cell_default.sib_sched_list = [
      ...enbConfig.nb_cell_default.sib_sched_list,
      {
        content: { // LTE SIB3 contents
          "message": {
            "c1": {
              "systemInformation-r13": {
                "criticalExtensions": {
                  "systemInformation-r13": {
                    "sib-TypeAndInfo-r13": [
                      {
                        "sib3-r13": {
                          "cellReselectionInfoCommon-r13": {
                            "q-Hyst-r13": "dB4"
                          },
                          "cellReselectionServingFreqInfo-r13": {
                            "s-NonIntraSearch-r13": 9
                            // "s-NonIntraSearch-r13": 31
                          },
                          "intraFreqCellReselectionInfo-r13": {
                            /* q-RxLevMin-r13 -60, */
                            // "q-RxLevMin-r13": -25,
                            // "q-RxLevMin-r13": -30,
                            "q-RxLevMin-r13": -64,
                            "s-IntraSearchP-r13": 29,
                            "t-Reselection-r13": "s6"
                            // "t-Reselection-r13": "s12"
                          }
                        }
                      }
                    ]
                  }
                }
              }
            }
          }
        },
        content_type: 'application/json', // JER encoding
        si_periodicity: 2048, // 2048 frames
        si_repetition_pattern: 4,
        si_value_tag: 2, /* increment modulo 4 if SIB is modified */
      }
    ]

    // enbConfig.nb_cell_list.map(e => e.cell_gain = -200);
    // enbConfig.nb_cell_list[0].cell_gain = 0;

    // Run Entities
    this.mme = await new Entity(mmeConfig);
    this.enb = await new Entity(enbConfig);

    this.enb.process.stdin.write('t spl\n');

    ///////
    await this.enb.remoteAPI({
      message: 'cell_gain',
      cell_id: 2,
      // gain: -50+i,
      gain: -200,
    });

    await this.enb.remoteAPI({
      message: 'cell_gain',
      cell_id: 12,
      // gain: -50+i,
      gain: -200,
    });

    logger.info('[TC7.1][Step 0] RF ON 및 초기 데이터콜 준비');
    await this.adapter.setRF(1);
    if (this.adapter.activateDataCall) {
      await waitMS(CONFIG.WAIT_AFTER_RF_ON_BEFORE_DATACALL_MS || 1000);
      try {
        await this.adapter.activateDataCall();
      } catch (err) {
        logger.info(`activateDataCall failed after RF on: ${err?.message || err}`);
      }
    }
    logger.info('[TC7.1][Step 0] 단계 결과: RF ON and data call trigger done');

    //----------------------------------------------------------------------------------------------
    // 7.1.2. 시험방법
    // 1) 단말이 특정 망에서 attach 성공 후 RRC connected 상태가 아닌 Idle 모드로 변경되어 있는지 확인한다.
    // 2) 시험 망의 TA 값을 변경하여 단말이 TAI 에 등록되어 있지 않은 TA 를 탐지하도록 준비한다.
    // *만약 시험환경이 여의치 않을 시 필드에서 확인한다.
    // 3) 단말이 tracking area update request 메시지를 망으로 송신하는지 확인한다. (EPS update type = TA updating 인지 확인한다)
    // 4) 단말이 망으로부터 GUTI 를 포함한 tracking area update accept 메시지를 수신하는지 확인한다.
    // 5) 단말을 idle 상태로 만든 후 과정 2)~5)를 반복하며, tracking area update 과정 이후 단말이 정상적으로 데이터 송/수신이 가능한지 확인한다
    // 7.1.3. 판정기준
    // 단말이 새로운 tracking area 를 탐지한 경우 tracking area update 를 정상적으로 수행하여야 한다
    //----------------------------------------------------------------------------------------------

    const criteriaTitle = '[판정기준] 새로운 TA 탐지 후 TAU 절차(Request/Accept/Complete) 정상 수행';
    const stopOnRequiredFail = CONFIG.TC_7_1_STOP_ON_REQUIRED_FAIL ?? CONFIG.TC_STOP_ON_REQUIRED_FAIL ?? false;
    const requiredFailures = [];

    const failCriteria = async (remarks) => {
      logger.info(`[TC7.1][FAIL] ${remarks}`);
      requiredFailures.push(remarks);
      if (stopOnRequiredFail) {
        this.addTestResult({ title: criteriaTitle, result: false, remarks });
        throw new Error(remarks);
      }
    };

    let attachAccepted = false;
    let rrcReleased = false;
    let tauUpdateTypeOk = false;
    let tauUpdateTypeText = undefined;
    let tauAcceptReceived = false;
    let gutiPresent = false;
    let tauCompleteReceived = false;

    //----------------------------------------------------------------------------------------------
    // Step 1) 단말이 특정 망에서 attach 성공 후 Idle 모드로 변경되어 있는지 확인한다.
    //----------------------------------------------------------------------------------------------
    logger.info('[TC7.1][Step 1] 단계 시작: 초기 Attach 완료 및 RRC Idle 전환 확인');
    const checkAttach = await checkEmmAttachProcedure(this, 120000);
    attachAccepted = checkAttach.emmAttachAccept !== undefined;
    logger.info(`[TC7.1][Step 1] Attach Accept=${attachAccepted}`);
    if (!attachAccepted) {
      await failCriteria('Step1 실패: Attach Accept 미수신');
    }

    //////////
    // rf 켜고

    ///////
    await this.enb.remoteAPI({
      message: 'cell_gain',
      cell_id: 12,
      // gain: 0,
      gain: -10,
    });

    await this.enb.remoteAPI({
      message: 'cell_gain',
      cell_id: 2,
      // gain: 0,
      gain: -10,
    });
    ////

    const rrcRelease = await this.enb.checkLog(30000, msg => {
      if (msg.data[0]?.includes('RRC connection release')) {
        return true;
      }
    });
    rrcReleased = rrcRelease === true;
    logger.info(`[TC7.1][Step 1] 단계 결과: AttachAccept=${attachAccepted}, RrcRelease=${rrcReleased}`);
    if (!rrcReleased) {
      await failCriteria('Step1 실패: RRC connection release 미수신 (Idle 전환 불확실)');
    }

    //----------------------------------------------------------------------------------------------
    // Step 2) 시험 망의 TA 값을 변경하여 단말이 TAI 에 등록되지 않은 TA 를 탐지하도록 준비한다.
    //----------------------------------------------------------------------------------------------
    logger.info('[TC7.1][Step 2] 단계 시작: cell gain 전환으로 타 TA 탐지 유도');
    await waitMS(1000);

    this.enb.process.stdin.write('t spl\n');

    this.enb.process.stdin.write('cell\n');
    this.enb.process.stdin.write('cell phy\n');

    // await waitMS(20000); // wait 20s for SIB3 scheduling.
    /////
    // for (let i=0; i<50; i++) { // 0 -> -200
    //  logger.info(`Set atten ${-(4 * i)} dB`);
  
    //  await this.enb.remoteAPI({
    //    message: 'cell_gain',
    //    cell_id: 1,
    //    // gain: -50+i,
    //    gain: -(4 * i),
    //  });
  
    //  await this.enb.remoteAPI({
    //    message: 'cell_gain',
    //    cell_id: 11,
    //    // gain: -50+i,
    //    gain: -(4 * i),
    //  });

    //  await waitMS(100);
    // }

    logger.info(`Set atten 45dB with delay`);
    await waitMS(20000); // wait 20s for SIB3 scheduling.
  
    await this.enb.remoteAPI({
      message: 'cell_gain',
      cell_id: 1,
      gain: -45,
    });

    await this.enb.remoteAPI({
      message: 'cell_gain',
      cell_id: 11,
      gain: -45,
    });

    this.enb.process.stdin.write('t spl\n');
    this.enb.process.stdin.write('cell\n');
    this.enb.process.stdin.write('cell phy\n');

    await waitMS(200);

    this.enb.process.stdin.write('t spl\n');
    this.enb.process.stdin.write('cell\n');
    this.enb.process.stdin.write('cell phy\n');

    logger.info('[TC7.1][Step 2] 단계 결과: cell_12 gain=0, cell_11 gain=-200, pass=true');

    //----------------------------------------------------------------------------------------------
    // Step 3) 단말이 TAU request (EPS update type = TA updating) 메시지를 송신하는지 확인한다.
    // Step 4) 단말이 GUTI 를 포함한 TAU accept 메시지를 수신하는지 확인한다.
    //----------------------------------------------------------------------------------------------
    logger.info('[TC7.1][Step 3/4] 단계 시작: TAU Request / Accept / Complete 확인');
    const checkTau = await checkEmmTauProcedure(this, 60000);
    // const checkTau = await checkEmmTauProcedure(this, 90000);
    // const checkTau = await checkEmmTauProcedure(this, 180000);
    tauAcceptReceived = checkTau.emmTrackingAreaUpdateAccept !== undefined;
    tauCompleteReceived = checkTau.emmTrackingAreaUpdateComplete !== undefined;

    if (checkTau.emmTrackingAreaUpdateRequest !== undefined) {
      const msgTauRequest = doTree(checkTau.emmTrackingAreaUpdateRequest.data);
      const findUpdateType = msgTauRequest.findChild('EPS update type:');
      tauUpdateTypeText = findUpdateType?.findText('Value = ');
      tauUpdateTypeOk = tauUpdateTypeText?.includes('TA updating') === true;
    }
    logger.info(`[TC7.1][Step 3] TauRequest=${checkTau.emmTrackingAreaUpdateRequest !== undefined}, UpdateType=${tauUpdateTypeText || 'N/A'}, pass=${tauUpdateTypeOk}`);

    if (checkTau.emmTrackingAreaUpdateRequest === undefined) {
      await failCriteria('Step3 실패: TAU Request 미수신');
    }
    if (!tauUpdateTypeOk) {
      await failCriteria('Step3 실패: EPS update type이 TA updating 아님');
    }

    if (tauAcceptReceived) {
      const msgTauAccept = doTree(checkTau.emmTrackingAreaUpdateAccept.data);
      const findGuti = msgTauAccept.findChild('GUTI:');
      gutiPresent = findGuti !== undefined;
      logger.info(`[TC7.1][Step 4] TauAccept=${tauAcceptReceived}, GUTI=${gutiPresent}`);
    } else {
      logger.info(`[TC7.1][Step 4] TauAccept=${tauAcceptReceived}`);
      await failCriteria('Step4 실패: TAU Accept 미수신');
    }
    if (!gutiPresent) {
      await failCriteria('Step4 실패: TAU Accept 내 GUTI 미포함');
    }

    //----------------------------------------------------------------------------------------------
    // Step 5) 단말이 TAU complete 메시지로 응답하는지 확인한다.
    //----------------------------------------------------------------------------------------------
    logger.info(`[TC7.1][Step 5] 단계 결과: TauComplete=${tauCompleteReceived}`);
    if (!tauCompleteReceived) {
      await failCriteria('Step5 실패: TAU Complete 미수신');
    }

    const criteriaPass =
      requiredFailures.length === 0
      && attachAccepted
      && tauUpdateTypeOk
      && tauAcceptReceived
      && gutiPresent
      && tauCompleteReceived;

    this.addTestResult({
      title: criteriaTitle,
      result: criteriaPass,
      remarks: `StopOnRequiredFail=${stopOnRequiredFail}, RequiredFailures=${requiredFailures.length > 0 ? requiredFailures.join(' | ') : 'none'}, AttachAccept=${attachAccepted}, RrcRelease=${rrcReleased}, TauUpdateTypeOk=${tauUpdateTypeOk}(${tauUpdateTypeText || 'N/A'}), TauAccept=${tauAcceptReceived}, GUTI=${gutiPresent}, TauComplete=${tauCompleteReceived}`,
    });
    await waitMS(CONFIG.WAIT_FOR_DEINIT_TEST_MS || 3000);
  }

  async deinit() {
    await this.log('End of Test, DUT turn off RF');
    await this.adapter.setRF(0);

    this.addTestLogs([this.mme /*, this.ims*/], { filterLayer: ['NAS', 'SIP'] });

    if (this.enb) {
      await this.enb.killEntity();
      delete this.enb;
    }

    if (this.mme) {
      await this.mme.killEntity();
      delete this.mme;
    }

    await super.deinit();
  }
}
