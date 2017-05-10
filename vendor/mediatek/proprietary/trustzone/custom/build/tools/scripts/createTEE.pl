#!/usr/bin/perl

use Getopt::Long;
use Cwd;

my $project_name = "";
my $project_cfg_file = "";
my $project_is_flavor = "no";
my $create_action = "on";
my $pwd = cwd();

&GetOptions(
  "prj_name=s" => \$project_name,
  "prj_cfg=s" => \$project_cfg_file,
  "flavor=s" => \$project_is_flavor,
  "action=s" => \$create_action,
);

if (($project_cfg_file eq "" ) || (!-e $project_cfg_file)) {
  print "ERROR: Can not find project config file $project_cfg_file\n";
  &usage();
}

# check project type (TEE must be main project)
if ($project_is_flavor eq "yes")
{
  die "ERROR: TEE can't create on flavor project\n";
}

# check kernel version and platform name in project config
my $kernel_version = getOptionValue($project_cfg_file, "LINUX_KERNEL_VERSION");
my $platform_name = getOptionValue($project_cfg_file, "MTK_PLATFORM");
my $trustonic_tee = getOptionValue($project_cfg_file, "TRUSTONIC_TEE_SUPPORT");
my $microtrust_tee = getOptionValue($project_cfg_file, "MICROTRUST_TEE_SUPPORT");
my $google_trusty = getOptionValue($project_cfg_file, "MTK_GOOGLE_TRUSTY_SUPPORT");
my $in_house_tee = getOptionValue($project_cfg_file, "MTK_IN_HOUSE_TEE_SUPPORT");
my $tee_support = getOptionValue($project_cfg_file, "MTK_TEE_SUPPORT");

print "INFO: kernel version is $kernel_version\n";
print "INFO: platform name is $platform_name\n";

# check for other option
if (($tee_support ne "yes") && ($tee_support ne "no")) {
  die "ERROR: MTK_TEE_SUPPORT is not configured!\n"
} elsif (($create_action eq "on") && ($tee_support ne "yes")) {
  die "ERROR: MTK_TEE_SUPPORT is not set to \"yes\"!\n"
} elsif (($create_action eq "off") && ($tee_support ne "no")) {
  die "ERROR: MTK_TEE_SUPPORT is not set to \"no\"!\n"
}

if (($create_action eq "on") && ($tee_support eq "yes")) {
  if (($trustonic_tee eq "yes") && ($microtrust_tee eq "yes")) {
    die "ERROR: TRUSTONIC_TEE_SUPPORT and MICROTRUST_TEE_SUPPORT can't set to \"yes\" in the same project!\n"
  } elsif (($trustonic_tee eq "yes") && ($google_trusty eq "yes")) {
    die "ERROR: TRUSTONIC_TEE_SUPPORT and MTK_GOOGLE_TRUSTY_SUPPORT can't set to \"yes\" in the same project!\n"
  } elsif (($google_trusty eq "yes") && ($microtrust_tee eq "yes")) {
    die "ERROR: MICROTRUST_TEE_SUPPORT and MTK_GOOGLE_TRUSTY_SUPPORT can't set to \"yes\" in the same project!\n"
  }
}

# check ARMv8 or ARMv7
my $arch_type = "Armv8";
if (($platform_name eq "MT6582") || ($platform_name eq "MT6592") || ($platform_name eq "MT6580"))
{
  $arch_type = "Armv7";
}

# check TEE type
my $tee_type = "";
if ($trustonic_tee eq "yes") {
  $tee_type = "trustonic";
} elsif($microtrust_tee eq "yes") {
  $tee_type = "teei";
} elsif($google_trusty eq "yes") {
  $tee_type = "trusty";
} elsif($in_house_tee eq "yes") {
  $tee_type = "mtee";
}

if (($create_action eq "on") && ($tee_type eq "")) {
  die "ERROR: TRUSTONIC_TEE_SUPPORT || MICROTRUST_TEE_SUPPORT || MTK_GOOGLE_TRUSTY_SUPPORT are not set to \"yes\"!\n"
}

if (($create_action eq "on") && ($in_house_tee eq "yes")) {
  print "INFO: Skip for MTK_IN_HOUSE_TEE_SUPPORT!\n";
  exit 0;
}

print "INFO: tee type is $tee_type\n";

if ($microtrust_tee eq "yes") {
  if($arch_type eq "Armv7") {
    die "ERROR: ARMv7 chipsets are not supported in MICROTRUST TEE!\n";
  }
  if (($platform_name eq "MT6735") || ($platform_name eq "MT6752") || ($platform_name eq "MT6795")) {
    die "ERROR: MT6735/MT6752/MT6795 chipsets are not supported in MICROTRUST TEE!\n";
  }
}

my $result1 = teeOnOffCheckOperation($trustonic_tee, "trustonic");
my $result2 = teeOnOffCheckOperation($microtrust_tee, "teei");
my $result3 = teeOnOffCheckOperation($google_trusty, "trusty");
my $result4 = "not an enabling action!\n";
if (($create_action eq "on") && ($tee_support eq "yes")) {
  $result4 = teeOnOffCheckOperation("yes", $tee_type);
}

print "======================================================\n";
print "======================================================\n";
print "[OP1]: ".$result1;
print "[OP2]: ".$result2;
print "[OP3]: ".$result3;
print "[OP4]: ".$result4;
print "======================================================\n";
print "======================================================\n";

exit 0;

sub trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };

sub teeOnOffCheckOperation{
  my $enable_tee_script = "./changeTEE.pl";
  my $config_str = "";
  my $project_str = "--project=".$project_name;
  my $kernel_str = "--kernel=".$kernel_version;
  my $change_tee_cmd = "";
  my $final_action = "no change";

  my $prj_tee_setting=$_[0];
  my $prj_tee_type=$_[1];

  if(($prj_tee_setting eq "no") || ($prj_tee_setting eq "yes")) {
    if (($create_action eq "on") && ($prj_tee_setting eq "yes")) {
      $final_action = "on";
    } else {
      $final_action = "off";
    }
    $config_str = "--config=".$prj_tee_type.$arch_type."_".$final_action.".xml";
    $change_tee_cmd = $enable_tee_script." ".$config_str." ".$project_str." ".$kernel_str;
    print "EXEC($final_action): $change_tee_cmd\n";
    system($change_tee_cmd);
    print "======================================================\n";
    print "$prj_tee_type options are turned $final_action successfully!\n";
    print "======================================================\n";
  }

  return "operation for ".$prj_tee_type." is ".$final_action."!\n";
}

sub getOptionValue{
  my $file=$_[0];
  my $search_str=$_[1];
  my $key;
  my $value;

  open MAKEFILE,"<$file" or die "Can not open $file\n";
  while(<MAKEFILE>){
    my $line = $_;
    chomp $line;
    if($line =~ /^\s*$search_str\s*(:?)=/){
      print "DBG: ".$line."\n";
      ($key, $value) = split /=/, $line;
      #print "key=".trim($key)."\n";
      #print "value=".trim($value)."\n";
    }
  }
  close MAKEFILE;

  return trim($value);
}

sub usage{
  warn << "__END_OF_USAGE";
Usage: ./createTEE.pl --prj_name=ProjectName --prj_cfg=ConfigFilePath --flavor=no --action=on
       ./createTEE.pl --prj_name=ProjectName --prj_cfg=ConfigFilePath --flavor=yes --action=on

Options:
  --prj_name    : project name
  --prj_cfg     : project config file path
  --flavor      : yes or no
  --action      : on or off

__END_OF_USAGE

  exit 1;
}
