
#undef IPC_OBJECT_TYPE_P_
#undef IPC_OBJECT_TYPE_I_

#define IPC_OBJECT_TYPE_P_ IPC_PP_JOIN_(IPC_OBJECT_TYPE_, _)
#define IPC_OBJECT_TYPE_I_ IPC_PP_JOIN_(IPC_OBJECT_TYPE_, _impl)

class IPC_OBJECT_TYPE_::IPC_OBJECT_TYPE_P_ : public pimpl<IPC_OBJECT_TYPE_P_> {
public:
    std::string n_;
    ipc::detail::IPC_OBJECT_TYPE_I_ h_;
};

void IPC_OBJECT_TYPE_::remove(char const * name) {
    detail::IPC_OBJECT_TYPE_I_::remove(name);
}

IPC_OBJECT_TYPE_::IPC_OBJECT_TYPE_()
    : p_(p_->make()) {
}

IPC_OBJECT_TYPE_::IPC_OBJECT_TYPE_(char const * name)
    : IPC_OBJECT_TYPE_() {
    open(name);
}

IPC_OBJECT_TYPE_::IPC_OBJECT_TYPE_(IPC_OBJECT_TYPE_&& rhs)
    : IPC_OBJECT_TYPE_() {
    swap(rhs);
}

IPC_OBJECT_TYPE_::~IPC_OBJECT_TYPE_() {
    close();
    p_->clear();
}

void IPC_OBJECT_TYPE_::swap(IPC_OBJECT_TYPE_& rhs) {
    std::swap(p_, rhs.p_);
}

IPC_OBJECT_TYPE_& IPC_OBJECT_TYPE_::operator=(IPC_OBJECT_TYPE_ rhs) {
    swap(rhs);
    return *this;
}

bool IPC_OBJECT_TYPE_::valid() const {
    return (p_ != nullptr) && !impl(p_)->n_.empty();
}

char const * IPC_OBJECT_TYPE_::name() const {
    return impl(p_)->n_.c_str();
}

bool IPC_OBJECT_TYPE_::open(char const * name IPC_OBJECT_TYPE_OPEN_PARS_) {
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    if (impl(p_)->n_ == name) return true;
    close();
    if (impl(p_)->h_.open(name IPC_OBJECT_TYPE_OPEN_ARGS_)) {
        impl(p_)->n_ = name;
        return true;
    }
    return false;
}

void IPC_OBJECT_TYPE_::close() {
    if (!valid()) return;
    impl(p_)->h_.close();
    impl(p_)->n_.clear();
}
