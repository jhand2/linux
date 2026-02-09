#include <linux/acpi.h>
#include <linux/tpm_eventlog.h>
#include "tpm.h"

// Doesn't support any actual TPM operations. This is a dummy chip to use
// when setting up the eventlog in sysfs.
static const struct tpm_class_ops tpm_eventlog_only = {};

static int tpm_eventlog_add(struct acpi_device *device)
{
	struct device *dev = &device->dev;
	struct tpm_chip *chip;
	chip = tpmm_chip_alloc(dev, &tpm_eventlog_only);
	if (IS_ERR(chip))
		return PTR_ERR(chip);

	chip->flags |= TPM_CHIP_FLAG_TPM2;
	chip->acpi_dev_handle = device->handle;

	tpm_sysfs_add_device(chip);

	tpm_bios_log_setup(chip);

	if (!chip->bin_log_seqops.chip)
		dev_warn(
			&chip->dev,
			"%s: failed to create binary BIOS log. No log found.\n",
			__func__);

	return 0;
}

static void tpm_eventlog_remove(struct acpi_device *device)
{
	struct device *dev = &device->dev;
	struct tpm_chip *chip = dev_get_drvdata(dev);

	tpm_bios_log_teardown(chip);
}

// Load this driver if the a TPM2 ACPI table with ID GOOGE109 is found. This
// is used to denote a TPM device which does not have a standard Linux
// interface but the boot stack still supports TPM event logs.
//
// If an event log is not found in the ACPI table, this driver will still
// attempt to use the EFI and OF methods to retrieve the log.
static const struct acpi_device_id tpm_eventlog_device_ids[] = {
	{ "GOOGE109", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, tpm_eventlog_device_ids);

static struct acpi_driver eventlog_acpi_driver = {
	.name = "tpm_eventlog_only",
	.ids = tpm_eventlog_device_ids,
	.ops = {
		.add = tpm_eventlog_add,
		.remove = tpm_eventlog_remove,
	},
};

module_acpi_driver(eventlog_acpi_driver);
MODULE_AUTHOR("Jordan Hand <jhand@google.com>");
MODULE_DESCRIPTION("TPM2 Driver which only exposes the TPM eventlog");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
