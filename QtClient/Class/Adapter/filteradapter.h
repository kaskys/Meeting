#ifndef FILTERADAPTER_H
#define FILTERADAPTER_H


class FilterAdapter
{
public:
    FilterAdapter();
    ~FilterAdapter();

    void* onInsertIpAddr(int);
    void onRemoveIpAddr();
    void onCheckIpAddr();
private:
};

#endif // FILTERADAPTER_H
