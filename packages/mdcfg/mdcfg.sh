#!/bin/sh

### Magic debconf stuff :-) ###
. /usr/share/debconf/confmodule

### Functions ###
md_get_devices() {
	DEVICES=""
	for i in `cat /proc/mdstat|grep ^md|sed -e 's/^\(md.*\) : active \([[:alnum:]]*\).*/\1_\2/'`; do
		if [ -z "$DEVICES" ]; then
			DEVICES="${i}"
		else
			DEVICES="${DEVICES}, ${i}"
		fi
	done
}

md_delete_verify() {
	DEVICE=`echo "$1" | sed -e "s/^\(md.*\)_.*/\1/"`
	INFO=`cat /proc/mdstat|grep ^${DEVICE}| sed -e "s/^md.* : active \([[:alnum:]]*\) \(.*\)/\1:\2/"`
	TYPE=`echo ${INFO}|sed -e "s/\(.*\):.*/\1/"`
	DEVICES=`echo ${INFO}|sed -e "s/.*:\(.*\)/\1/"`
	NUMBER=`echo ${DEVICE}|sed -e "s/^md//"`
	db_set mdcfg/deleteverify "false"
	db_fset mdcfg/deleteverify "seen" "false"
	db_subst mdcfg/deleteverify DEVICE "/dev/${DEVICE}"
	db_subst mdcfg/deleteverify TYPE "${TYPE}"
	db_subst mdcfg/deleteverify DEVICES "${DEVICES}"
	db_input high mdcfg/deleteverify
	db_go
	db_get mdcfg/deleteverify

	case $RET in 
		"true")
			# Stop the MD device, and zero the superblock
			# of all the component devices
			#echo "mdadm --stop /dev/${DEVICE}"
			mdadm --stop /dev/md/${NUMBER}
			for DEV in $DEVICES; do
				DEV=`echo ${DEV}|sed -e "s/\\[.*\\]//g"`
				mdadm --zero-superblock "/dev/${DEV}"
				#echo "mdadm --zero-superblock /dev/${DEV}"
			done
			;;
		"false") ;;
	esac
}

md_delete() {
	md_get_devices
	db_set mdcfg/deletemenu "false"
	db_fset mdcfg/deletemenu "seen" "false"
	db_subst mdcfg/deletemenu DEVICES "${DEVICES}"
	db_input high mdcfg/deletemenu
	db_go
	db_get mdcfg/deletemenu

	case $RET in
		md*)
			md_delete_verify $RET;;
		*);;
	esac
}

md_createmain() {
	db_set mdcfg/createmain "false"
	db_fset mdcfg/createmain "seen" "false"
	db_input high mdcfg/createmain
	db_go
	if [ $? -ne "30" ]; then
		db_get mdcfg/createmain

		case $RET in
			"RAID1")
			md_create_raid1;;
		esac
	fi
}

md_create_raid1() {
	PARTITIONS=""

	# Get a list of RAID partitions. This only works if there is no 
	# filesystem on the partitions, which is fine by us.
	RAW_PARTITIONS=`/usr/lib/partconf/find-partitions 2>/dev/null|grep RAID|cut -f1`;

	# Convert it into a proper list form for a select question 
	# (comma seperated)
	NUM_PART=0
	for i in ${RAW_PARTITIONS}; do
		DEV=`echo ${i}|sed -e "s/\/dev\///"`
		cat /proc/mdstat | grep -q "${DEV}"

		if [ "$?" -eq "0" ] ; then
			continue
		fi

		if [ -z "${PARTITIONS}" ] ; then
			PARTITIONS="${i}"
		else
			PARTITIONS="${PARTITIONS}, ${i}"
		fi
		NUM_PART=$(($NUM_PART + 1))
	done

	if [ -z "${PARTITIONS}" ] ; then
		db_fset mdcfg/noparts "seen" "false"
		db_input critical mdcfg/noparts
		db_go mdcfg/noparts
		return
	fi

	OK=0

	db_set mdcfg/raid1devcount "2"

	# Get the count of active devices
	while [ "${OK}" -eq 0 ]; do
		db_fset mdcfg/raid1devcount "seen" "false"
		db_input high mdcfg/raid1devcount
		db_go
		if [ "$?" -eq "30" ]; then
			# If the user has pressed "Cancel", return
			return
		fi

		# Figure out, if the user entered a number
		db_get mdcfg/raid1devcount
		RET=`echo ${RET}|sed -e "s/[[:space:]]//g"`
		if [ ! -z "${RET}" ]; then
			let "OK=${RET}>0 && ${RET}<99"
		fi
	done

	db_set mdcfg/raid1sparecount "0"
	OK=0

	# Same procedure as above
	# TODO: Make a general function for this kind of stuff
	while [ "${OK}" -eq 0 ]; do
		db_fset mdcfg/raid1sparecount "seen" "false"
		db_input high mdcfg/raid1sparecount
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi
		db_get mdcfg/raid1sparecount
		RET=`echo ${RET}|sed -e "s/[[:space:]]//g"`
		if [ ! -z "${RET}" ]; then
			let "OK=${RET}>=0 && ${RET}<99"
		fi
	done

	db_get mdcfg/raid1devcount
	DEV_COUNT="${RET}"
	db_get mdcfg/raid1sparecount
	SPARE_COUNT="${RET}"
	REQUIRED=$(($DEV_COUNT + $SPARE_COUNT))
	if [ "$REQUIRED" -gt "$NUM_PART" ] ; then
		db_fset mdcfg/notenoughparts "seen" "false"
		db_subst mdcfg/raid1devs NUM_PART "${NUM_PART}"
		db_subst mdcfg/raid1devs REQUIRED "${REQUIRED}"
		db_subst mdcfg/raid1devs DEV "${DEV_COUNT}"
		db_subst mdcfg/raid1devs SPARE "${SPARE_COUNT}"
		db_input critical mdcfg/notenoughparts
		db_go mdcfg/notenoughparts
		return
	fi

	db_set mdcfg/raid1devs ""
	SELECTED=0

	# Loop until the correct amount of devices has been selected
	while [ "${SELECTED}" -ne "${DEV_COUNT}" ]; do
		db_fset mdcfg/raid1devs "seen" "false"
		db_subst mdcfg/raid1devs COUNT "${DEV_COUNT}"
		db_subst mdcfg/raid1devs PARTITIONS "${PARTITIONS}"
		db_input high mdcfg/raid1devs
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi

		db_get mdcfg/raid1devs
		SELECTED=0
		for i in $RET; do
			DEVICE=`echo ${i}|sed -e "s/,//"`
			let SELECTED++
		done
	done

	# Remove partitions selected in raid1devs from the PARTITION list
	db_get mdcfg/raid1devs

	PARTITIONS=`echo ${PARTITIONS}|sed -e "s/,/\\n/g"`

	for i in $RET; do
		DEVICE=`echo ${i}|sed -e "s/,//"`
		PARTITIONS=`echo -e ${PARTITIONS}|sed -e "s/ /\\n/g"|grep -v "${DEVICE}"`
	done


	db_set mdcfg/raid1sparedevs ""
	SELECTED=99

	if [ "${SPARE_COUNT}" -eq "0" ]; then
		SELECTED=0
	fi

	# Loop until the correct amount of devices has been selected
	while [ "${SELECTED}" -gt "${SPARE_COUNT}" ]; do
		db_fset mdcfg/raid1sparedevs "seen" "false"
		db_subst mdcfg/raid1sparedevs COUNT "${SPARE_COUNT}"
		db_subst mdcfg/raid1sparedevs PARTITIONS "${PARTITIONS}"
		db_input high mdcfg/raid1sparedevs
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi

		db_get mdcfg/raid1sparedevs
		SELECTED=0
		for i in $RET; do
			DEVICE=`echo ${i}|sed -e "s/,//"`
			let SELECTED++
		done
	done

	# The amount of spares, the user has selected
	NAMED_SPARES=${SELECTED}

	db_get mdcfg/raid1devs
	RAID_DEVICES=`echo ${RET}|sed -e "s/,//g"`

	db_get mdcfg/raid1sparedevs
	SPARE_DEVICES=`echo ${RET}|sed -e "s/,//g"`

	MISSING_SPARES=""

	COUNT=${NAMED_SPARES}
	while [ "${COUNT}" -lt "${SPARE_COUNT}" ]; do
		MISSING_SPARES="${MISSING_SPARES} missing"
		let COUNT++
	done

	# Find the next available md-number
	MD_NUM=`cat /proc/mdstat|grep ^md|sed -e 's/^md\(.*\) : active .*/\1/'|sort|tail -1`
	if [ -z "${MD_NUM}" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	echo "Selected spare count: ${NAMED_SPARES}"
	echo "Raid devices count: ${DEV_COUNT}"
	echo "Spare devices count: ${SPARE_COUNT}"
	echo "Commandline:"
	`mdadm --create /dev/md/${MD_NUM} --force -l raid1 -n ${DEV_COUNT} -x ${SPARE_COUNT} ${RAID_DEVICES} ${SPARE_DEVICES} ${MISSING_SPARES}`
}

md_mainmenu() {
	while [ 1 ]; do
		db_set mdcfg/mainmenu "false"
		db_fset mdcfg/mainmenu "seen" "false"
		db_input high mdcfg/mainmenu
		db_go
		if [ $? -eq "30" ]; then
			exit 30
		fi
		db_get mdcfg/mainmenu
		case $RET in
			"Create MD device")
				md_createmain;;
			"Delete MD device")
				md_delete;;
			"Finish")
				break;;
		esac
	done
}

### Main of script ###

# Try to load the necesarry modules.
# As we only support raid1 at this point, only that module is loaded.
depmod -a 1>/dev/null 2>&1
modprobe md 1>/dev/null 2>&1
modprobe raid1 1>/dev/null 2>&1

# Try to detect MD devices, and start them
/sbin/mdrun

# Make sure that we have md-support
if [ ! -e /proc/mdstat ]; then
	db_set mdcfg/nomd "false"
	db_input high mdcfg/nomd
	db_go
	db_stop
	exit 0
fi

# Force mdadm to be installed on the target system
apt-install mdadm

# We want the "go back" button
#db_capb backup

md_mainmenu

#db_stop
exit 0
