module Svc {
  struct SeqArgs {
    $size: FwSizeType
    buffer: [SequenceArgumentsMaxSize] U8
  } default { $size = 0, buffer = 0 }

  @ Sequencer blocking state
  enum BlockState : U8 {
    BLOCK = 0
    NO_BLOCK = 1
  }

  @ Port to request a sequence be run
  port CmdSeqIn(
                 filename: string size FileNameStringSize @< The sequence file
                 args: SeqArgs                            @< Sequence arguments
               )

  @ Port to cancel a sequence
  port CmdSeqCancel

  @ Port for querying the i-th upcoming row of a sequence schedule table, 0 being the soonest
  @ Returns true when the reply is valid
  port SeqSchedulerQuery(
                          i: FwSizeType @< Position among the upcoming rows, 0 being the soonest
                          ref timeDue: Fw.Time @< Time the row is due
                          ref fileName: string size FileNameStringSize @< Sequence file of the row
                        ) -> bool

}
