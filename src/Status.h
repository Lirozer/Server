enum class Status
{
    Success = 0,

    SocketCreationFailed,
    SocketBindFailed,
    SocketListenFailed,

    EpollCreationFailed,
    EpollAddFailed
};