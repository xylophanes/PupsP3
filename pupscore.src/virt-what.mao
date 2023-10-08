# Check for Docker (MAO /proc/1/cgroup is best way to detect docker)
isdocker=$(cat /proc/1/mounts | head -1 | grep docker)
if [ "$isdocker" != "" ] ; then
    echo docker
fi
