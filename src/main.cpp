#include <string>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <getopt.h>
#include <thread>

#include <zlib.h>
#include "kseq.h"

#ifndef KSEQ_INIT_READY
#define KSEQ_INIT_READY
KSEQ_INIT(gzFile, gzread)
#endif


#include "common.h"
#include "ProcessReads.h"
#include "KmerIndex.h"
#include "Kmer.hpp"
#include "MinCollector.h"
#include "EMAlgorithm.h"
#include "weights.h"
#include "Inspect.h"
#include "Bootstrap.h"
#include "H5Writer.h"


using namespace std;


void ParseOptionsIndex(int argc, char **argv, ProgramOptions& opt) {
  int verbose_flag = 0;

  const char *opt_string = "i:k:f:";
  static struct option long_options[] = {
    // long args
    {"verbose", no_argument, &verbose_flag, 1},
    // short args
    {"index", required_argument, 0, 'i'},
    {"kmer-size", required_argument, 0, 'k'},
    {"trans-fasta", required_argument, 0, 'f'},
    {0,0,0,0}
  };
  int c;
  int option_index = 0;
  while (true) {
    c = getopt_long(argc,argv,opt_string, long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 'i': {
      opt.index = optarg;
      break;
    }
    case 'k': {
      stringstream(optarg) >> opt.k;
      break;
    }
    case 'f': {
      opt.transfasta = optarg;
      break;
    }
    default: break;
    }
  }

  if (verbose_flag) {
    opt.verbose = true;
  }
}

void ParseOptionsInspect(int argc, char **argv, ProgramOptions& opt) {
  int verbose_flag = 0;

  const char *opt_string = "i:";
  static struct option long_options[] = {
    // long args
    {"verbose", no_argument, &verbose_flag, 1},
    // short args
    {"index", required_argument, 0, 'i'},
    {0,0,0,0}
  };
  int c;
  int option_index = 0;
  while (true) {
    c = getopt_long(argc,argv,opt_string, long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 'i': {
      opt.index = optarg;
      break;
    }
    default: break;
    }
  }

  if (verbose_flag) {
    opt.verbose = true;
  }
}



void ParseOptionsEM(int argc, char **argv, ProgramOptions& opt) {
  int verbose_flag = 0;

  const char *opt_string = "t:i:s:l:o:n:m:d:b:";
  static struct option long_options[] = {
    // long args
    {"verbose", no_argument, &verbose_flag, 1},
    // short args
    {"seed", required_argument, 0, 'd'},
    {"threads", required_argument, 0, 't'},
    {"index", required_argument, 0, 'i'},
    {"skip", required_argument, 0, 's'},
    {"fragment-length", required_argument, 0, 'l'},
    {"output-dir", required_argument, 0, 'o'},
    {"iterations", required_argument, 0, 'n'},
    {"min-range", required_argument, 0, 'm'},
    {"bootstrap-samples", required_argument, 0, 'b'},
    {0,0,0,0}
  };
  int c;
  int option_index = 0;
  while (true) {
    c = getopt_long(argc,argv,opt_string, long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 't': {
      stringstream(optarg) >> opt.threads;
      break;
    }
    case 'i': {
      opt.index = optarg;
      break;
    }
    case 's': {
      stringstream(optarg) >> opt.skip;
      break;
    }
    case 'l': {
      stringstream(optarg) >> opt.fld;
      break;
    }
    case 'o': {
      opt.output = optarg;
      break;
    }
    case 'n': {
      stringstream(optarg) >> opt.iterations;
      break;
    }
    case 'm': {
      stringstream(optarg) >> opt.min_range;
    }
    case 'b': {
      stringstream(optarg) >> opt.bootstrap;
      break;
    }
    case 'd': {
      stringstream(optarg) >> opt.seed;
      break;
    }
    default: break;
    }
  }

  // all other arguments are fast[a/q] files to be read
  for (int i = optind; i < argc; i++) {
    opt.files.push_back(argv[i]);
  }

  if (verbose_flag) {
    opt.verbose = true;
  }
}

void ParseOptionsEMOnly(int argc, char **argv, ProgramOptions& opt) {
  int verbose_flag = 0;

  const char *opt_string = "t:s:l:o:n:m:d:b:";
  static struct option long_options[] = {
    // long args
    {"verbose", no_argument, &verbose_flag, 1},
    {"seed", required_argument, 0, 'd'},
    // short args
    {"threads", required_argument, 0, 't'},
    {"fragment-length", required_argument, 0, 'l'},
    {"output-dir", required_argument, 0, 'o'},
    {"iterations", required_argument, 0, 'n'},
    {"min-range", required_argument, 0, 'm'},
    {"bootstrap-samples", required_argument, 0, 'b'},
    {0,0,0,0}
  };
  int c;
  int option_index = 0;
  while (true) {
    c = getopt_long(argc,argv,opt_string, long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 't': {
      stringstream(optarg) >> opt.threads;
      break;
    }
    case 'l': {
      stringstream(optarg) >> opt.fld;
      break;
    }
    case 'o': {
      opt.output = optarg;
      break;
    }
    case 'n': {
      stringstream(optarg) >> opt.iterations;
      break;
    }
    case 'm': {
      stringstream(optarg) >> opt.min_range;
    }
    case 'b': {
      stringstream(optarg) >> opt.bootstrap;
      break;
    }
    case 'd': {
      stringstream(optarg) >> opt.seed;
      break;
    }
    default: break;
    }
  }

  if (verbose_flag) {
    opt.verbose = true;
  }

}


bool CheckOptionsIndex(ProgramOptions& opt) {

  bool ret = true;

  if (opt.k <= 0 || opt.k >= Kmer::MAX_K) {
    cerr << "Error: invalid k-mer size " << opt.k << ", maximum is " << (Kmer::MAX_K -1) << endl;
    ret = false;
  }

  // we want to generate the index, check k, index and transfasta
  struct stat stFileInfo;
  auto intStat = stat(opt.transfasta.c_str(), &stFileInfo);
  if (intStat != 0) {
    cerr << "Error: transcript fasta file not found " << opt.transfasta << endl;
    ret = false;
  }

  if (opt.index.empty()) {
    cerr << "Error: need to specify index name" << endl;
    ret = false;
  }

  return ret;
}

bool CheckOptionsEM(ProgramOptions& opt, bool emonly = false) {

  bool ret = true;


  // check for index
  if (!emonly) {
    if (opt.index.empty()) {
      cerr << "Error: index file missing" << endl;
      ret = false;
    } else {
      struct stat stFileInfo;
      auto intStat = stat(opt.index.c_str(), &stFileInfo);
      if (intStat != 0) {
        cerr << "Error: index file not found " << opt.index << endl;
        ret = false;
      }
    }
  }

  // check for read files
  if (!emonly) {
    if (opt.files.size() == 0) {
      cerr << "Error: Missing read files" << endl;
      ret = false;
    } else {
      struct stat stFileInfo;
      for (auto& fn : opt.files) {
        auto intStat = stat(fn.c_str(), &stFileInfo);
        if (intStat != 0) {
          cerr << "Error: file not found " << fn << endl;
          ret = false;
        }
      }
    }

    if (!(opt.files.size() == 1 || opt.files.size() == 2)) {
      cerr << "Error: Input files should be 1 or 2 files only" << endl;
      ret = false;
    }

    if (opt.skip <= 0) {
      cerr << "Error: skip has to be a positive integer" << endl;
      ret = false;
    }
  }

  if (opt.fld == 0.0) {
    // In the future, if we have single-end data we should require this
    // argument
    cerr << "[quant] Mean fragment length not provided. Will estimate from data" << endl;
  }

  if (opt.fld < 0.0) {
    cerr << "Error: invalid value for mean fragment length " << opt.fld << endl;
    ret = false;
  }

  if (opt.iterations <= 0) {
    cerr << "Error: Invalid number of iterations " << opt.iterations << endl;
    ret = false;
  }

  if (opt.min_range <= 0) {
    cerr << "Error: Invalid value for minimum range " << opt.min_range << endl;
    ret = false;
  }

  if (opt.output.empty()) {
    cerr << "Error: need to specify output directory " << opt.output << endl;
    ret = false;
  } else {
    struct stat stFileInfo;
    auto intStat = stat(opt.output.c_str(), &stFileInfo);
    if (intStat == 0) {
      // file/dir exits
      if (!S_ISDIR(stFileInfo.st_mode)) {
        cerr << "Error: file " << opt.output << " exists and is not a directory" << endl;
        ret = false;
      } else if (emonly) {
        // check for directory/counts.txt
        struct stat stCountInfo;
        auto intcountstat = stat((opt.output + "/counts.txt" ).c_str(), &stCountInfo);
        if (intcountstat != 0) {
          cerr << "Error: could not find file " << opt.output << "/counts.txt" << endl;
          ret = false;
        }

        // check for directory/index.saved
        struct stat stIndexInfo;
        auto intindexstat = stat((opt.output + "/index.saved").c_str(), &stIndexInfo);
        if (intindexstat != 0) {
          cerr << "Error: could not find index " << opt.output << "/index.saved" << endl;
          ret = false;
        }
        opt.index = (opt.output + "/index.saved");
      }
    } else {
      if (emonly) {
        cerr << "Error: output directory needs to exist, run quant first" << endl;
        ret = false;
      } else {
        // create directory
        if (mkdir(opt.output.c_str(), 0777) == -1) {
          cerr << "Error: could not create directory " << opt.output << endl;
          ret = false;
        }
      }
    }
  }

  if (opt.threads <= 0) {
    cerr << "Error: invalid number of threads " << opt.threads << endl;
    ret = false;
  } else {
    unsigned int n = std::thread::hardware_concurrency();
    if (n != 0 && n < opt.threads) {
      cerr << "Warning: you asked for " << opt.threads
           << ", but only " << n << " cores on the machine" << endl;
    }
  }

  if (opt.bootstrap < 0) {
    cerr << "Error: number of bootstrap samples must be a non-negative integer." << endl;
    ret = false;
  }

  return ret;
}


bool CheckOptionsInspect(ProgramOptions& opt) {

  bool ret = true;
  // check for index
  if (opt.index.empty()) {
    cerr << "Error: index file missing" << endl;
    ret = false;
  } else {
    struct stat stFileInfo;
    auto intStat = stat(opt.index.c_str(), &stFileInfo);
    if (intStat != 0) {
      cerr << "Error: index file not found " << opt.index << endl;
      ret = false;
    }
  }

  return ret;
}



void PrintCite() {
  cout << "The paper describing this software has not been published." << endl;
  //  cerr << "When using this program in your research, please cite" << endl << endl;
}

void PrintVersion() {
  cout << "Kallisto, version: " << 	KALLISTO_VERSION << endl;
}

void usage() {
  cout << "Kallisto " << endl
       << "Does transcriptome stuff" << endl << endl
       << "Usage: Kallisto CMD [options] .." << endl << endl
       << "Where <CMD> can be one of:" << endl << endl
       << "    index         Builds the index "<< endl
       << "    inspect       Inspects a built index "<< endl
       << "    quant         Runs the quantification algorithm " << endl
       << "    quant-only    Runs the quantification algorithm without mapping" << endl
       << "    cite          Prints citation information " << endl
       << "    version       Prints version information"<< endl << endl;
}


void usageIndex() {
  cout << "Kallisto " << endl
       << "Does transcriptome stuff" << endl << endl
       << "Usage: Kallisto index [options]" << endl << endl
       << "-k, --kmer-size=INT         Size of k-mers, default (21), max value is " << (Kmer::MAX_K-1) << endl
       << "-i, --index=STRING             Filename for index to be constructed " << endl
       << "-f, --trans-fasta=STRING       FASTA file containing reference transcriptome " << endl
       << "    --verbose               Print lots of messages during run" << endl;
}

void usageInspect() {
  cout << "Kallisto " << endl
       << "Does transcriptome stuff" << endl << endl
       << "Usage: Kallisto inspect [options]" << endl << endl
       << "-i, --index=STRING             Filename for index to inspect " << endl
       << "    --verbose               Print lots of messages during run" << endl;
}

void usageEM() {
  cout << "Kallisto " << endl
       << "Does transcriptome stuff" << endl << endl
       << "Usage: Kallisto quant [options] FASTQ-files" << endl << endl
       << "-t, --threads=INT             Number of threads to use (default value 1)" << endl
       << "-i, --index=INT               Filename for index " << endl
       << "-s, --skip=INT                Number of k-mers to skip (default value 1)" << endl
       << "-l, --fragment-length=DOUBLE  Estimated fragment length (default values are estimated from data)" << endl
       << "-m, --min-range               Minimum range to assign a read to a transcript (default value 2*k+1)" << endl
       << "-n, --iterations=INT          Number of iterations of EM algorithm (default value 500)" << endl
       << "-b, --bootstrap-samples=INT   Number of bootstrap samples to perform (default value 0)" << endl
       << "--seed=INT                    Seed for bootstrap samples (default value 42)" << endl
       << "-o, --output-dir=STRING       Directory to store output to" << endl
       << "    --verbose                 Print lots of messages during run" << endl;
}

void usageEMOnly() {
  cout << "Kallisto " << endl
       << "Does transcriptome stuff" << endl << endl
       << "Usage: Kallisto quant-only [options]" << endl << endl
       << "-t, --threads=INT             Number of threads to use (default value 1)" << endl
       << "-s, --skip=INT                Number of k-mers to skip (default value 1)" << endl
       << "-l, --fragment-length=DOUBLE  Estimated fragment length (default values are estimated from data)" << endl
       << "-m, --min-range               Minimum range to assign a read to a transcript (default value 2*k+1)" << endl
       << "-n, --iterations=INT          Number of iterations of EM algorithm (default value 500)" << endl
       << "-b, --bootstrap-samples=INT   Number of bootstrap samples to perform (default value 0)" << endl
       << "--seed=INT                    Seed for bootstrap samples (default value 42)" << endl
       << "-o, --output-dir=STRING       Directory to store output to" << endl
       << "    --verbose                 Print lots of messages during run" << endl;
}

void write_version(const std::string& fname) {
  std::ofstream out;
  out.open(fname, std::ios::out);

  if (!out.is_open()) {
    cerr << "Error opening '" << fname << "'" << endl;
    exit(1);
  }

  out << KALLISTO_VERSION;

  out.close();
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    usage();
    exit(1);
  } else {
    ProgramOptions opt;
    string cmd(argv[1]);
    if (cmd == "version") {
      PrintVersion();
    } else if (cmd == "cite") {
      PrintCite();
    } else if (cmd == "index") {
      if (argc==2) {
        usageIndex();
        return 0;
      }
      ParseOptionsIndex(argc-1,argv+1,opt);
      if (!CheckOptionsIndex(opt)) {
        usageIndex();
        exit(1);
      } else {
        // create an index
        Kmer::set_k(opt.k);
        KmerIndex index(opt);
        std::cerr << "Building index from: " << opt.transfasta << std::endl;
        index.BuildTranscripts(opt);
        index.write(opt.index);
      }
    } else if (cmd == "inspect") {
      if (argc==2) {
        usageInspect();
        return 0;
      }
      ParseOptionsInspect(argc-1, argv+1, opt);
      if (!CheckOptionsInspect(opt)) {
        usageInspect();
        exit(1);
      } else {
        KmerIndex index(opt);
        index.load(opt);
        InspectIndex(index);
      }

    } else if (cmd == "quant") {
      if (argc==2) {
        usageEM();
        return 0;
      }
      ParseOptionsEM(argc-1,argv+1,opt);
      if (!CheckOptionsEM(opt)) {
        usageEM();
        exit(1);
      } else {
        write_version(opt.output + "/kallisto_version.txt");
        // run the em algorithm
        KmerIndex index(opt);
        index.load(opt);

        // temporarily build index
        /* opt.transfasta = "../test/input/10_trans_gt_500_bp.fasta"; */
        // std::cerr << "Building index from: " << opt.transfasta << std::endl;
        // index.BuildTranscripts(opt);

        auto firstFile = opt.files[0];
        MinCollector collection(index, opt);
        if (firstFile.size() >= 4 && firstFile.compare(firstFile.size()-4,4,".bam") == 0) {
          ProcessBams<KmerIndex, MinCollector>(index, opt, collection);
        } else {
          ProcessReads<KmerIndex, MinCollector>(index, opt, collection);
        }
        // save modified index for future use
        index.write((opt.output+"/index.saved"), false);
        // if mean FL not provided, estimate
        auto mean_fl = (opt.fld > 0.0) ? opt.fld : get_mean_frag_len(collection);
        std::cerr << "Estimated mean fragment length: " << mean_fl << std::endl;
        auto eff_lens = calc_eff_lens(index.trans_lens_, mean_fl);
        auto weights = calc_weights (collection.counts, index.ecmap, eff_lens);
        EMAlgorithm em(index.ecmap, collection.counts, index.target_names_,
                       eff_lens, weights);
        em.run();
        em.compute_rho();
        em.write(opt.output + "/expression.txt");

        H5Writer writer(opt.output + "/expression.h5", opt.bootstrap, 6);
        writer.write_main(em, index.target_names_, index.trans_lens_);

        if (opt.bootstrap > 0) {
          std::cerr << "Bootstrapping!" << std::endl;
          auto B = opt.bootstrap;
          std::mt19937_64 rand;
          rand.seed( opt.seed );

          std::vector<size_t> seeds;
          for (auto s = 0; s < B; ++s) {
            seeds.push_back( rand() );
          }

          for (auto b = 0; b < B; ++b) {
            Bootstrap bs(collection.counts, index.ecmap,
                         index.target_names_, eff_lens, seeds[b]);
            std::cerr << "Running EM bootstrap: " << b << std::endl;
            auto res = bs.run_em(em);
            writer.write_bootstrap(res, b);
            // res.write( opt.output + "/bs_expression_" + std::to_string(b) +
            //            ".txt");
          }

        }
      }
    } else if (cmd == "quant-only") {
      if (argc==2) {
        usageEMOnly();
        return 0;
      }
      ParseOptionsEMOnly(argc-1,argv+1,opt);
      if (!CheckOptionsEM(opt, true)) {
        usageEMOnly();
        exit(1);
      } else {
        write_version(opt.output + "/kallisto_version.txt");
        // run the em algorithm
        KmerIndex index(opt);
        index.load(opt, false); // skip the k-mer map
        MinCollector collection(index, opt);
        collection.loadCounts(opt);
        // if mean FL not provided, estimate
        auto mean_fl = (opt.fld > 0.0) ? opt.fld : get_mean_frag_len(collection);
        std::cerr << "Estimated mean fragment length: " << mean_fl << std::endl;
        auto eff_lens = calc_eff_lens(index.trans_lens_, mean_fl);
        auto weights = calc_weights (collection.counts, index.ecmap, eff_lens);


        EMAlgorithm em(index.ecmap, collection.counts, index.target_names_,
                       eff_lens, weights);

        em.run();

        H5Writer writer(opt.output + "/expression.h5", opt.bootstrap, 7);
        writer.write_main(em, index.target_names_, index.trans_lens_);

        if (opt.bootstrap > 0) {
          std::cerr << "Bootstrapping!" << std::endl;
          auto B = opt.bootstrap;
          std::mt19937_64 rand;
          rand.seed( opt.seed );

          std::vector<size_t> seeds;
          for (auto s = 0; s < B; ++s) {
            seeds.push_back( rand() );
          }

          for (auto b = 0; b < B; ++b) {
            Bootstrap bs(collection.counts, index.ecmap,
                         index.target_names_, eff_lens, seeds[b]);
            std::cerr << "Running EM bootstrap: " << b << std::endl;
            auto res = bs.run_em(em);
            writer.write_bootstrap(res, b);
            // res.write( opt.output + "/bs_expression_" + std::to_string(b) +
            //            ".txt");
          }

        }
      }
    } else {
      cerr << "Did not understand command " << cmd << endl;
      usage();
      exit(1);
    }

  }
  return 0;
}
